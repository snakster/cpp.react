
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_GRAPH_OBSERVERNODES_H_INCLUDED
#define REACT_DETAIL_GRAPH_OBSERVERNODES_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <memory>
#include <utility>

#include "GraphBase.h"

#include "react/detail/ReactiveInput.h"

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
    typename S,
    typename TFunc
>
class SignalObserverNode : public ObserverNode<D>
{
    using Engine = typename SignalObserverNode::Engine;

public:
    template <typename F>
    SignalObserverNode(const std::shared_ptr<SignalNode<D,S>>& subject, F&& func) :
        SignalObserverNode::ObserverNode( ),
        subject_( subject ),
        func_( std::forward<F>(func) )
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

        GlobalObserverState<>::ShouldDetach = false;

        ContinuationHolder<D>::SetTurn(turn);

        if (auto p = subject_.lock())
            func_(p->ValueRef());

        ContinuationHolder<D>::Clear();

        if (GlobalObserverState<>::ShouldDetach)
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

    std::weak_ptr<SignalNode<D,S>>  subject_;
    TFunc                           func_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventObserverNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename E,
    typename TFunc
>
class EventObserverNode : public ObserverNode<D>
{
    using Engine = typename EventObserverNode::Engine;

public:
    template <typename F>
    EventObserverNode(const std::shared_ptr<EventStreamNode<D,E>>& subject, F&& func) :
        EventObserverNode::ObserverNode( ),
        subject_( subject ),
        func_( std::forward<F>(func) )
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
        
        GlobalObserverState<>::ShouldDetach = false;

        ContinuationHolder<D>::SetTurn(turn);

        if (auto p = subject_.lock())
        {
            for (const auto& e : p->Events())
                func_(e);
        }

        ContinuationHolder<D>::Clear();

        if (GlobalObserverState<>::ShouldDetach)
            turn.QueueForDetach(*this);

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));
    }

private:
    std::weak_ptr<EventStreamNode<D,E>> subject_;

    TFunc   func_;

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
    typename E,
    typename TFunc,
    typename ... TDepValues
>
class SyncedObserverNode : public ObserverNode<D>
{
    using Engine = typename SyncedObserverNode::Engine;

public:
    // NOTE: After upgrading to VS2013 Udpate2, using std::shared_ptr here crashes the compiler
    template <typename F>
    SyncedObserverNode(const std::shared_ptr<EventStreamNode<D,E>>& subject, F&& func, 
                       const std::shared_ptr<SignalNode<D,TDepValues>>& ... deps) :
        SyncedObserverNode::ObserverNode( ),
        subject_( subject ),
        func_( std::forward<F>(func) ),
        deps_( deps ... )
    {
        Engine::OnNodeCreate(*this);
        subject->IncObsCount();
        Engine::OnNodeAttach(*this, *subject);

        REACT_EXPAND_PACK(Engine::OnNodeAttach(*this, *deps));
    }

    ~SyncedObserverNode()
    {
        Engine::OnNodeDestroy(*this);
    }

    virtual const char* GetNodeType() const override        { return "SyncedObserverNode"; }
    virtual int         DependencyCount() const override    { return 1 + sizeof...(TDepValues); }

    virtual void Tick(void* turnPtr) override
    {
        struct EvalFunctor_
        {
            EvalFunctor_(const E& e, TFunc& f) :
                MyEvent( e ),
                MyFunc( f )
            {}

            void operator()(const std::shared_ptr<SignalNode<D,TDepValues>>& ... args)
            {
                MyFunc(MyEvent, args->ValueRef() ...);
            }

            const E&    MyEvent;
            TFunc&      MyFunc;
        };

        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));
        
        GlobalObserverState<>::ShouldDetach = false;

        ContinuationHolder<D>::SetTurn(turn);

        if (auto p = subject_.lock())
        {
            // Update of this node could be triggered from deps,
            // so make sure source doesnt contain events from last turn
            p->SetCurrentTurn(turn);

            for (const auto& e : p->Events())
                apply(EvalFunctor_{ e, func_ }, deps_);
        }

        ContinuationHolder<D>::Clear();

        if (GlobalObserverState<>::ShouldDetach)
            turn.QueueForDetach(*this);

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));
    }

private:
    using DepHolderT = std::tuple<std::shared_ptr<SignalNode<D,TDepValues>>...>;

    std::weak_ptr<EventStreamNode<D,E>> subject_;
    
    DepHolderT  deps_;
    TFunc       func_;

    virtual void detachObserver()
    {
        if (auto p = subject_.lock())
        {
            p->DecObsCount();

            Engine::OnNodeDetach(*this, *p);

            apply(
                DetachFunctor<D,SyncedObserverNode,
                    std::shared_ptr<SignalNode<D,TDepValues>>...>( *this ),
                deps_);

            subject_.reset();
        }
    }
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_GRAPH_OBSERVERNODES_H_INCLUDED