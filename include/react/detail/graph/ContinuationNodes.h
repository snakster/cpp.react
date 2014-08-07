
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_GRAPH_CONTINUATIONNODES_H_INCLUDED
#define REACT_DETAIL_GRAPH_CONTINUATIONNODES_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <functional>
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
/// AddContinuationRangeWrapper
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E, typename F, typename ... TArgs>
struct AddContinuationRangeWrapper
{
    AddContinuationRangeWrapper(const AddContinuationRangeWrapper& other) = default;

    AddContinuationRangeWrapper(AddContinuationRangeWrapper&& other) :
        MyFunc( std::move(other.MyFunc) )
    {}

    template
    <
        typename FIn,
        class = typename DisableIfSame<FIn,AddContinuationRangeWrapper>::type
    >
    explicit AddContinuationRangeWrapper(FIn&& func) :
        MyFunc( std::forward<FIn>(func) )
    {}

    void operator()(EventRange<E> range, const TArgs& ... args)
    {
        for (const auto& e : range)
            MyFunc(e, args ...);
    }

    F MyFunc;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ContinuationNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class ContinuationNode : public NodeBase<D>
{
public:
    ContinuationNode(TransactionFlagsT turnFlags) :
        turnFlags_( turnFlags )
    {}

    virtual bool IsOutputNode() const { return true; }

protected:
    TransactionFlagsT turnFlags_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SignalContinuationNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename DOut,
    typename S,
    typename TFunc
>
class SignalContinuationNode : public ContinuationNode<D>
{
    using Engine = typename SignalContinuationNode::Engine;

public:
    template <typename F>
    SignalContinuationNode(TransactionFlagsT turnFlags,
                           const std::shared_ptr<SignalNode<D,S>>& trigger, F&& func) :
        SignalContinuationNode::ContinuationNode( turnFlags ),
        trigger_( trigger ),
        func_( std::forward<F>(func) )
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *trigger);
    }

    ~SignalContinuationNode()
    {
        Engine::OnNodeDestroy(*this);
    }

    virtual const char* GetNodeType() const override        { return "SignalContinuationNode"; }
    virtual int         DependencyCount() const override    { return 1; }

    virtual void Tick(void* turnPtr) override
    {
#ifdef REACT_ENABLE_LOGGING
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);
#endif

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        auto& storedValue = trigger_->ValueRef();
        auto& storedFunc = func_;

        TransactionFuncT cont
        (
            // Copy value and func
            [storedFunc,storedValue] () mutable
            {
                storedFunc(storedValue);
            }
        );

        DomainSpecificInputManager<D>::Instance()
            .StoreContinuation(
                DomainSpecificInputManager<DOut>::Instance(),
                this->turnFlags_,
                std::move(cont));

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));
    }

private:
    std::shared_ptr<SignalNode<D,S>> trigger_;

    TFunc   func_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventContinuationNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename DOut,
    typename E,
    typename TFunc
>
class EventContinuationNode : public ContinuationNode<D>
{
    using Engine = typename EventContinuationNode::Engine;

public:
    template <typename F>
    EventContinuationNode(TransactionFlagsT turnFlags,
                          const std::shared_ptr<EventStreamNode<D,E>>& trigger, F&& func) :
        EventContinuationNode::ContinuationNode( turnFlags ),
        trigger_( trigger ),
        func_( std::forward<F>(func) )
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *trigger);
    }

    ~EventContinuationNode()
    {
        Engine::OnNodeDestroy(*this);
    }

    virtual const char* GetNodeType() const override        { return "EventContinuationNode"; }
    virtual int         DependencyCount() const override    { return 1; }

    virtual void Tick(void* turnPtr) override
    {
#ifdef REACT_ENABLE_LOGGING
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);
#endif

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        auto& storedEvents = trigger_->Events();
        auto& storedFunc = func_;

        TransactionFuncT cont
        (
            // Copy events and func
            [storedFunc,storedEvents] () mutable
            {
                storedFunc(EventRange<E>( storedEvents ));
            }
        );

        DomainSpecificInputManager<D>::Instance()
            .StoreContinuation(
                DomainSpecificInputManager<DOut>::Instance(),
                this->turnFlags_,
                std::move(cont));

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));
    }

private:
    std::shared_ptr<EventStreamNode<D,E>> trigger_;

    TFunc   func_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncedContinuationNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename DOut,
    typename E,
    typename TFunc,
    typename ... TDepValues
>
class SyncedContinuationNode : public ContinuationNode<D>
{
    using Engine = typename SyncedContinuationNode::Engine;

    using ValueTupleT = std::tuple<TDepValues...>;

    struct TupleBuilder_
    {
        ValueTupleT operator()(const std::shared_ptr<SignalNode<D,TDepValues>>& ... deps)
        {
            return ValueTupleT(deps->ValueRef() ...);
        }
    };

    struct EvalFunctor_
    {
        EvalFunctor_(const E& e, TFunc& f) :
            MyEvent( e ),
            MyFunc( f )
        {}

        void operator()(const TDepValues& ... vals)
        {
            MyFunc(MyEvent, vals ...);
        }

        const E&    MyEvent;
        TFunc&      MyFunc;
    };

public:
    template <typename F>
    SyncedContinuationNode(TransactionFlagsT turnFlags,
                           const std::shared_ptr<EventStreamNode<D,E>>& trigger, F&& func, 
                           const std::shared_ptr<SignalNode<D,TDepValues>>& ... deps) :
        SyncedContinuationNode::ContinuationNode( turnFlags ),
        trigger_( trigger ),
        func_( std::forward<F>(func) ),
        deps_( deps ... )
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *trigger);

        REACT_EXPAND_PACK(Engine::OnNodeAttach(*this, *deps));
    }

    ~SyncedContinuationNode()
    {
        Engine::OnNodeDetach(*this, *trigger_);

        apply(
            DetachFunctor<D,SyncedContinuationNode,
                std::shared_ptr<SignalNode<D,TDepValues>>...>( *this ),
            deps_);

        Engine::OnNodeDestroy(*this);
    }

    virtual const char* GetNodeType() const override        { return "SyncedContinuationNode"; }
    virtual int         DependencyCount() const override    { return 1 + sizeof...(TDepValues); }

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        // Update of this node could be triggered from deps,
        // so make sure source doesnt contain events from last turn
        trigger_->SetCurrentTurn(turn);

        auto& storedEvents = trigger_->Events();
        auto& storedFunc = func_;

        // Copy values to tuple
        ValueTupleT storedValues = apply(TupleBuilder_( ), deps_);

        // Note: MSVC error, if using () initialization.
        // Probably a compiler bug.
        TransactionFuncT cont
        {
            // Copy events, func, value tuple (note: 2x copy)
            [storedFunc,storedEvents,storedValues] () mutable
            {
                apply(
                    [&storedFunc,&storedEvents] (const TDepValues& ... vals)
                    {
                        storedFunc(EventRange<E>( storedEvents ), vals ...);
                    },
                    storedValues);
            }
        };

        DomainSpecificInputManager<D>::Instance()
            .StoreContinuation(
                DomainSpecificInputManager<DOut>::Instance(),
                this->turnFlags_,
                std::move(cont));

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));
    }

private:
    using DepHolderT = std::tuple<std::shared_ptr<SignalNode<D,TDepValues>>...>;

    std::shared_ptr<EventStreamNode<D,E>> trigger_;
    
    TFunc       func_;
    DepHolderT  deps_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_GRAPH_CONTINUATIONNODES_H_INCLUDED