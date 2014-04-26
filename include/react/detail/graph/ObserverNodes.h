
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/detail/Defs.h"

#include <memory>
#include <functional>

#include "EventStreamNodes.h"
#include "GraphBase.h"
#include "SignalNodes.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

// tbb tasks are non-preemptible, thread local flag for each worker
namespace current_observer_state_
{
    static __declspec(thread) bool    shouldDetach = false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ObserverNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class ObserverNode :
    public ReactiveNode<D,void,void>,
    public IObserverNode
{
public:
    using PtrT = std::shared_ptr<ObserverNode>;

    ObserverNode() :
        ReactiveNode()
    {}

    virtual const char* GetNodeType() const     { return "ObserverNode"; }
    virtual bool        IsOutputNode() const    { return true; }
};

template <typename D>
using ObserverNodePtr = typename ObserverNode<D>::PtrT;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SignalObserverNode
///////////////////////////////////////////////////////////////////////////////////////////////////
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
    SignalObserverNode(const SignalNodePtr<D,TArg>& subject, F&& func) :
        ObserverNode<D>(),
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

    virtual const char* GetNodeType() const override { return "SignalObserverNode"; }

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

    virtual int DependencyCount() const    { return 1; }

    virtual void Detach()
    {
        if (auto p = subject_.lock())
        {
            p->DecObsCount();

            Engine::OnNodeDetach(*this, *p);
            subject_.reset();
        }
    }

private:
    SignalNodeWeakPtr<D,TArg>   subject_;

    TFunc   func_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventObserverNode
///////////////////////////////////////////////////////////////////////////////////////////////////
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
    EventObserverNode(const EventStreamNodePtr<D,TArg>& subject, F&& func) :
        ObserverNode<D>(),
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

    virtual const char* GetNodeType() const override { return "EventObserverNode"; }

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

    virtual int DependencyCount() const    { return 1; }

    virtual void Detach()
    {
        if (auto p = subject_.lock())
        {
            p->DecObsCount();

            Engine::OnNodeDetach(*this, *p);
            subject_.reset();
        }
    }

private:
    EventStreamNodeWeakPtr<D,TArg>      subject_;
    TFunc   func_;
};

/****************************************/ REACT_IMPL_END /***************************************/
