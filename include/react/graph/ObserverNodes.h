
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/Defs.h"

#include <memory>
#include <functional>

#include "GraphBase.h"
#include "EventStreamNodes.h"
#include "SignalNodes.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

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

    explicit ObserverNode(bool registered) :
        ReactiveNode<D,void,void>(true)
    {
    }

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
    typename TArg
>
class SignalObserverNode : public ObserverNode<D>
{
public:
    template <typename F>
    SignalObserverNode(const SignalNodePtr<D,TArg>& subject, F&& func, bool registered) :
        ObserverNode<D>(true),
        subject_{ subject },
        func_{ std::forward<F>(func) }
    {
        if (!registered)
            registerNode();

        subject->IncObsCount();

        Engine::OnNodeAttach(*this, *subject);
    }

    virtual const char* GetNodeType() const override { return "SignalObserverNode"; }

    virtual ETickResult Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *static_cast<TurnT*>(turnPtr);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        current_observer_state_::shouldDetach = false;

        D::SetCurrentContinuation(turn);

        if (auto p = subject_.lock())
            func_(p->ValueRef());

        D::ClearCurrentContinuation();

        if (current_observer_state_::shouldDetach)
            turn.QueueForDetach(*this);

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        return ETickResult::none;
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
    SignalNodeWeakPtr<D,TArg>           subject_;
    const std::function<void(TArg)>     func_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventObserverNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename TArg
>
class EventObserverNode : public ObserverNode<D>
{
public:
    template <typename F>
    EventObserverNode(const EventStreamNodePtr<D,TArg>& subject, F&& func, bool registered) :
        ObserverNode<D>(true),
        subject_{ subject },
        func_{ std::forward<F>(func) }
    {
        if (!registered)
            registerNode();

        subject->IncObsCount();

        Engine::OnNodeAttach(*this, *subject);
    }

    virtual const char* GetNodeType() const override { return "EventObserverNode"; }

    virtual ETickResult Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *static_cast<TurnT*>(turnPtr);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));
        
        current_observer_state_::shouldDetach = false;

        D::SetCurrentContinuation(turn);

        if (auto p = subject_.lock())
        {
            for (const auto& e : p->Events())
                func_(e);
        }

        D::ClearCurrentContinuation();

        if (current_observer_state_::shouldDetach)
            turn.QueueForDetach(*this);

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        return ETickResult::none;
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
    const std::function<void(TArg)>     func_;
};

/****************************************/ REACT_IMPL_END /***************************************/
