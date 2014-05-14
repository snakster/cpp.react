
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
/// Forward declarations
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename S>
class SignalNode;

template <typename D, typename E>
class EventStreamNode;

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
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

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
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

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

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncedObserverNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename TValue,
    typename TFunc,
    typename ... TDepValues
>
class SyncedObserverNode : public ObserverNode<D>
{
public:
    template <typename F>
    SyncedObserverNode(const SharedPtrT<EventStreamNode<D,TValue>>& subject, F&& func, 
                       const SharedPtrT<SignalNode<D,TDepValues>>& ... depArgs) :
        ObserverNode{ },
        subject_{ subject },
        func_{ std::forward<F>(func) },
        deps_{ depArgs ... }
    {
        Engine::OnNodeCreate(*this);
        subject->IncObsCount();
        Engine::OnNodeAttach(*this, *subject);

        REACT_EXPAND_PACK(D::Engine::OnNodeAttach(*this, *depArgs));
    }

    ~SyncedObserverNode()
    {
        Engine::OnNodeDestroy(*this);
    }

    virtual const char* GetNodeType() const override        { return "SyncedEventObserverNode"; }
    virtual int         DependencyCount() const override    { return 1; }

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));
        
        current_observer_state_::shouldDetach = false;

        ContinuationHolder<D>::SetTurn(turn);

        if (auto p = subject_.lock())
        {
            for (const auto& e : p->Events())
                apply(EvalFunctor{ e, func_ }, deps_);
        }

        ContinuationHolder<D>::Clear();

        if (current_observer_state_::shouldDetach)
            turn.QueueForDetach(*this);

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));
    }

private:
    struct EvalFunctor
    {
        EvalFunctor(const TValue& e, TFunc& f) :
            MyEvent{ e },
            MyFunc{ f }
        {}

        void operator()(const SharedPtrT<SignalNode<D,TDepValues>>& ... args)
        {
            MyFunc(MyEvent, args->ValueRef() ...);
        }

        const TValue&   MyEvent;
        TFunc&          MyFunc;
    };

    struct DetachFunctor
    {
        DetachFunctor(SyncedObserverNode& node) : MyNode{ node }
        {}

        void operator()(const SharedPtrT<SignalNode<D,TDepValues>>& ... deps)
        {
            REACT_EXPAND_PACK(D::Engine::OnNodeDetach(MyNode, *deps));
        }

        SyncedObserverNode& MyNode;
    };

    using DepHolderT = std::tuple<SharedPtrT<SignalNode<D,TDepValues>>...>;

    WeakPtrT<EventStreamNode<D,TValue>>   subject_;
    
    DepHolderT  deps_;
    TFunc       func_;

    virtual void detachObserver()
    {
        if (auto p = subject_.lock())
        {
            p->DecObsCount();

            Engine::OnNodeDetach(*this, *p);
            apply(DetachFunctor{ *this }, deps_);

            subject_.reset();
        }
    }
};

/****************************************/ REACT_IMPL_END /***************************************/
