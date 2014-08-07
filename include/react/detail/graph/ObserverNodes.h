
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
/// ObserverAction
///////////////////////////////////////////////////////////////////////////////////////////////////
enum class ObserverAction
{
    next,
    stop_and_detach
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// AddObserverRangeWrapper
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E, typename F, typename ... TArgs>
struct AddObserverRangeWrapper
{
    AddObserverRangeWrapper(const AddObserverRangeWrapper& other) = default;

    AddObserverRangeWrapper(AddObserverRangeWrapper&& other) :
        MyFunc( std::move(other.MyFunc) )
    {}

    template
    <
        typename FIn,
        class = typename DisableIfSame<FIn,AddObserverRangeWrapper>::type
    >
    explicit AddObserverRangeWrapper(FIn&& func) :
        MyFunc( std::forward<FIn>(func) )
    {}

    ObserverAction operator()(EventRange<E> range, const TArgs& ... args)
    {
        for (const auto& e : range)
            if (MyFunc(e, args ...) == ObserverAction::stop_and_detach)
                return ObserverAction::stop_and_detach;

        return ObserverAction::next;
    }

    F MyFunc;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ObserverNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class ObserverNode :
    public NodeBase<D>,
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
class SignalObserverNode :
    public ObserverNode<D>
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
#ifdef REACT_ENABLE_LOGGING
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);
#endif

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        bool shouldDetach = false;

        if (auto p = subject_.lock())
        {// timer
            using TimerT = typename SignalObserverNode::ScopedUpdateTimer;
            TimerT scopedTimer( *this, 1 );
            
            if (func_(p->ValueRef()) == ObserverAction::stop_and_detach)
                shouldDetach = true;
        }// ~timer

        if (shouldDetach)
            DomainSpecificInputManager<D>::Instance()
                .QueueObserverForDetach(*this);

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));
    }

    virtual void UnregisterSelf() override
    {
        if (auto p = subject_.lock())
            p->UnregisterObserver(this);
    }

private:
    virtual void detachObserver() override
    {
        if (auto p = subject_.lock())
        {
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
class EventObserverNode :
    public ObserverNode<D>
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
#ifdef REACT_ENABLE_LOGGING
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);
#endif

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        bool shouldDetach = false;

        if (auto p = subject_.lock())
        {// timer
            using TimerT = typename EventObserverNode::ScopedUpdateTimer;
            TimerT scopedTimer( *this, p->Events().size() );

            shouldDetach = func_(EventRange<E>( p->Events() )) == ObserverAction::stop_and_detach;

        }// ~timer

        if (shouldDetach)
            DomainSpecificInputManager<D>::Instance()
                .QueueObserverForDetach(*this);

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));
    }

    virtual void UnregisterSelf() override
    {
        if (auto p = subject_.lock())
            p->UnregisterObserver(this);
    }

private:
    std::weak_ptr<EventStreamNode<D,E>> subject_;

    TFunc   func_;

    virtual void detachObserver()
    {
        if (auto p = subject_.lock())
        {
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
class SyncedObserverNode :
    public ObserverNode<D>
{
    using Engine = typename SyncedObserverNode::Engine;

public:
    template <typename F>
    SyncedObserverNode(const std::shared_ptr<EventStreamNode<D,E>>& subject, F&& func, 
                       const std::shared_ptr<SignalNode<D,TDepValues>>& ... deps) :
        SyncedObserverNode::ObserverNode( ),
        subject_( subject ),
        func_( std::forward<F>(func) ),
        deps_( deps ... )
    {
        Engine::OnNodeCreate(*this);
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
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));
        
        bool shouldDetach = false;

        if (auto p = subject_.lock())
        {
            // Update of this node could be triggered from deps,
            // so make sure source doesnt contain events from last turn
            p->SetCurrentTurn(turn);

            {// timer
                using TimerT = typename SyncedObserverNode::ScopedUpdateTimer;
                TimerT scopedTimer( *this, p->Events().size() );
            
                shouldDetach = apply(
                    [this, &p] (const std::shared_ptr<SignalNode<D,TDepValues>>& ... args)
                    {
                        return func_(EventRange<E>( p->Events() ), args->ValueRef() ...);
                    },
                    deps_) == ObserverAction::stop_and_detach;

            }// ~timer
        }

        if (shouldDetach)
            DomainSpecificInputManager<D>::Instance()
                .QueueObserverForDetach(*this);

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));
    }

    virtual void UnregisterSelf() override
    {
        if (auto p = subject_.lock())
            p->UnregisterObserver(this);
    }

private:
    using DepHolderT = std::tuple<std::shared_ptr<SignalNode<D,TDepValues>>...>;

    std::weak_ptr<EventStreamNode<D,E>> subject_;
    
    TFunc       func_;
    DepHolderT  deps_;

    virtual void detachObserver()
    {
        if (auto p = subject_.lock())
        {
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