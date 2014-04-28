
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/detail/Defs.h"

#include <utility>

#include "GraphBase.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ObserverNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class ObserverNode :
    public ReactiveNode<D,void,void>,
    public IObserver
{
public:
    ObserverNode() = default;

    virtual bool IsOutputNode() const { return true; }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SignalObserverNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename S>
class SignalNode;

template
<
    typename D,
    typename TArg,
    typename TFunc
>
class SignalObserverNode : public ObserverNode<D>
{
public:
    template <typename F>
    SignalObserverNode(const SharedPtrT<SignalNode<D,TArg>>& subject, F&& func) :
        ObserverNode{ },
        subject_{ subject },
        func_{ std::forward<F>(func) }
    {
        Engine::OnNodeCreate(*this);
        subject->IncObsCount();
        Engine::OnNodeAttach(*this, *subject);
    }

    ~SignalObserverNode()
    {
        Engine::OnNodeDestroy(*this);
    }

    virtual const char* GetNodeType() const override        { return "SignalObserverNode"; }
    virtual int         DependencyCount() const override    { return 1; }

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *static_cast<TurnT*>(turnPtr);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        current_observer_state_::shouldDetach = false;

        ContinuationHolder<D>::SetTurn(turn);

        if (auto p = subject_.lock())
            func_(p->ValueRef());

        ContinuationHolder<D>::Clear();

        if (current_observer_state_::shouldDetach)
            turn.QueueForDetach(*this);

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));
    }

private:
    virtual void detachObserver() override
    {
        if (auto p = subject_.lock())
        {
            p->DecObsCount();

            Engine::OnNodeDetach(*this, *p);
            subject_.reset();
        }
    }

    WeakPtrT<SignalNode<D,TArg>>    subject_;
    TFunc                           func_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventObserverNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename E>
class EventStreamNode;

template
<
    typename D,
    typename TArg,
    typename TFunc
>
class EventObserverNode : public ObserverNode<D>
{
public:
    template <typename F>
    EventObserverNode(const SharedPtrT<EventStreamNode<D,TArg>>& subject, F&& func) :
        ObserverNode{ },
        subject_{ subject },
        func_{ std::forward<F>(func) }
    {
        Engine::OnNodeCreate(*this);
        subject->IncObsCount();
        Engine::OnNodeAttach(*this, *subject);
    }

    ~EventObserverNode()
    {
        Engine::OnNodeDestroy(*this);
    }

    virtual const char* GetNodeType() const override        { return "EventObserverNode"; }
    virtual int         DependencyCount() const override    { return 1; }

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *static_cast<TurnT*>(turnPtr);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));
        
        current_observer_state_::shouldDetach = false;

        ContinuationHolder<D>::SetTurn(turn);

        if (auto p = subject_.lock())
        {
            for (const auto& e : p->Events())
                func_(e);
        }

        ContinuationHolder<D>::Clear();

        if (current_observer_state_::shouldDetach)
            turn.QueueForDetach(*this);

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));
    }

private:
    WeakPtrT<EventStreamNode<D,TArg>>   subject_;
    TFunc                               func_;

    virtual void detachObserver()
    {
        if (auto p = subject_.lock())
        {
            p->DecObsCount();

            Engine::OnNodeDetach(*this, *p);
            subject_.reset();
        }
    }
};

/****************************************/ REACT_IMPL_END /***************************************/
