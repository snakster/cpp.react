
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_GRAPH_ALGORITHMNODES_H_INCLUDED
#define REACT_DETAIL_GRAPH_ALGORITHMNODES_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <memory>
#include <utility>

#include "EventNodes.h"
#include "SignalNodes.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// AddIterateRangeWrapper
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E, typename S, typename F, typename ... TArgs>
struct AddIterateRangeWrapper
{
    AddIterateRangeWrapper(const AddIterateRangeWrapper& other) = default;

    AddIterateRangeWrapper(AddIterateRangeWrapper&& other) :
        MyFunc( std::move(other.MyFunc) )
    {}

    template
    <
        typename FIn,
        class = typename DisableIfSame<FIn,AddIterateRangeWrapper>::type
    >
    explicit AddIterateRangeWrapper(FIn&& func) :
        MyFunc( std::forward<FIn>(func) )
    {}

    S operator()(EventRange<E> range, S value, const TArgs& ... args)
    {
        for (const auto& e : range)
            value = MyFunc(e, value, args ...);

        return value;
    }

    F MyFunc;
};

template <typename E, typename S, typename F, typename ... TArgs>
struct AddIterateByRefRangeWrapper
{
    AddIterateByRefRangeWrapper(const AddIterateByRefRangeWrapper& other) = default;

    AddIterateByRefRangeWrapper(AddIterateByRefRangeWrapper&& other) :
        MyFunc( std::move(other.MyFunc) )
    {}

    template
    <
        typename FIn,
        class = typename DisableIfSame<FIn,AddIterateByRefRangeWrapper>::type
    >
    explicit AddIterateByRefRangeWrapper(FIn&& func) :
        MyFunc( std::forward<FIn>(func) )
    {}

    void operator()(EventRange<E> range, S& valueRef, const TArgs& ... args)
    {
        for (const auto& e : range)
            MyFunc(e, valueRef, args ...);
    }

    F MyFunc;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IterateNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S,
    typename E,
    typename TFunc
>
class IterateNode :
    public SignalNode<D,S>
{
    using Engine = typename IterateNode::Engine;

public:
    template <typename T, typename F>
    IterateNode(T&& init, const std::shared_ptr<EventStreamNode<D,E>>& events, F&& func) :
        IterateNode::SignalNode( std::forward<T>(init) ),
        events_( events ),
        func_( std::forward<F>(func) )
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *events);
    }

    ~IterateNode()
    {
        Engine::OnNodeDetach(*this, *events_);
        Engine::OnNodeDestroy(*this);
    }

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        bool changed = false;

        {// timer
            using TimerT = typename IterateNode::ScopedUpdateTimer;
            TimerT scopedTimer( *this, events_->Events().size() );

            S newValue = func_(EventRange<E>( events_->Events() ), this->value_);

            if (! Equals(newValue, this->value_))
            {
                this->value_ = std::move(newValue);
                changed = true;
            }
        }// ~timer

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (changed)
            Engine::OnNodePulse(*this, turn);
        else
            Engine::OnNodeIdlePulse(*this, turn);
    }

    virtual const char* GetNodeType() const override        { return "IterateNode"; }
    virtual int         DependencyCount() const override    { return 1; }

private:
    std::shared_ptr<EventStreamNode<D,E>> events_;
    
    TFunc   func_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IterateByRefNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S,
    typename E,
    typename TFunc
>
class IterateByRefNode :
    public SignalNode<D,S>
{
    using Engine = typename IterateByRefNode::Engine;

public:
    template <typename T, typename F>
    IterateByRefNode(T&& init, const std::shared_ptr<EventStreamNode<D,E>>& events, F&& func) :
        IterateByRefNode::SignalNode( std::forward<T>(init) ),
        func_( std::forward<F>(func) ),
        events_( events )
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *events);
    }

    ~IterateByRefNode()
    {
        Engine::OnNodeDetach(*this, *events_);
        Engine::OnNodeDestroy(*this);
    }

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        {// timer
            using TimerT = typename IterateByRefNode::ScopedUpdateTimer;
            TimerT scopedTimer( *this, events_->Events().size() );

            func_(EventRange<E>( events_->Events() ), this->value_);

        }// ~timer

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        // Always assume change
        Engine::OnNodePulse(*this, turn);
    }

    virtual const char* GetNodeType() const override        { return "IterateByRefNode"; }
    virtual int         DependencyCount() const override    { return 1; }

protected:
    TFunc   func_;

    std::shared_ptr<EventStreamNode<D,E>> events_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncedIterateNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S,
    typename E,
    typename TFunc,
    typename ... TDepValues
>
class SyncedIterateNode :
    public SignalNode<D,S>
{
    using Engine = typename SyncedIterateNode::Engine;

public:
    template <typename T, typename F>
    SyncedIterateNode(T&& init, const std::shared_ptr<EventStreamNode<D,E>>& events, F&& func,
                      const std::shared_ptr<SignalNode<D,TDepValues>>& ... deps) :
        SyncedIterateNode::SignalNode( std::forward<T>(init) ),
        events_( events ),
        func_( std::forward<F>(func) ),
        deps_( deps ... )
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *events);
        REACT_EXPAND_PACK(Engine::OnNodeAttach(*this, *deps));
    }

    ~SyncedIterateNode()
    {
        Engine::OnNodeDetach(*this, *events_);

        apply(
            DetachFunctor<D,SyncedIterateNode,
                std::shared_ptr<SignalNode<D,TDepValues>>...>( *this ),
            deps_);

        Engine::OnNodeDestroy(*this);
    }

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        events_->SetCurrentTurn(turn);

        bool changed = false;

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        if (! events_->Events().empty())
        {// timer
            using TimerT = typename SyncedIterateNode::ScopedUpdateTimer;
            TimerT scopedTimer( *this, events_->Events().size() );
            
            S newValue = apply(
                [this] (const std::shared_ptr<SignalNode<D,TDepValues>>& ... args)
                {
                    return func_(EventRange<E>( events_->Events() ), this->value_, args->ValueRef() ...);
                },
                deps_);

            if (! Equals(newValue, this->value_))
            {
                changed = true;
                this->value_ = std::move(newValue);
            }            
        }// ~timer

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (changed)   
            Engine::OnNodePulse(*this, turn);
        else
            Engine::OnNodeIdlePulse(*this, turn);
    }

    virtual const char* GetNodeType() const override        { return "SyncedIterateNode"; }
    virtual int         DependencyCount() const override    { return 1 + sizeof...(TDepValues); }

private:
    using DepHolderT = std::tuple<std::shared_ptr<SignalNode<D,TDepValues>>...>;

    std::shared_ptr<EventStreamNode<D,E>> events_;
    
    TFunc       func_;
    DepHolderT  deps_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncedIterateByRefNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S,
    typename E,
    typename TFunc,
    typename ... TDepValues
>
class SyncedIterateByRefNode :
    public SignalNode<D,S>
{
    using Engine = typename SyncedIterateByRefNode::Engine;

public:
    template <typename T, typename F>
    SyncedIterateByRefNode(T&& init, const std::shared_ptr<EventStreamNode<D,E>>& events, F&& func,
                           const std::shared_ptr<SignalNode<D,TDepValues>>& ... deps) :
        SyncedIterateByRefNode::SignalNode( std::forward<T>(init) ),
        events_( events ),
        func_( std::forward<F>(func) ),
        deps_( deps ... )
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *events);
        REACT_EXPAND_PACK(Engine::OnNodeAttach(*this, *deps));
    }

    ~SyncedIterateByRefNode()
    {
        Engine::OnNodeDetach(*this, *events_);

        apply(
            DetachFunctor<D,SyncedIterateByRefNode,
                std::shared_ptr<SignalNode<D,TDepValues>>...>( *this ),
            deps_);

        Engine::OnNodeDestroy(*this);
    }

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        events_->SetCurrentTurn(turn);

        bool changed = false;

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        if (! events_->Events().empty())
        {// timer
            using TimerT = typename SyncedIterateByRefNode::ScopedUpdateTimer;
            TimerT scopedTimer( *this, events_->Events().size() );

            apply(
                [this] (const std::shared_ptr<SignalNode<D,TDepValues>>& ... args)
                {
                    func_(EventRange<E>( events_->Events() ), this->value_, args->ValueRef() ...);
                },
                deps_);

            changed = true;
        }// ~timer

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (changed)
            Engine::OnNodePulse(*this, turn);
        else
            Engine::OnNodeIdlePulse(*this, turn);
    }

    virtual const char* GetNodeType() const override        { return "SyncedIterateByRefNode"; }
    virtual int         DependencyCount() const override    { return 1 + sizeof...(TDepValues); }

private:
    using DepHolderT = std::tuple<std::shared_ptr<SignalNode<D,TDepValues>>...>;

    std::shared_ptr<EventStreamNode<D,E>> events_;
    
    TFunc       func_;
    DepHolderT  deps_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// HoldNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S
>
class HoldNode : public SignalNode<D,S>
{
    using Engine = typename HoldNode::Engine;

public:
    template <typename T>
    HoldNode(T&& init, const std::shared_ptr<EventStreamNode<D,S>>& events) :
        HoldNode::SignalNode( std::forward<T>(init) ),
        events_( events )
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *events_);
    }

    ~HoldNode()
    {
        Engine::OnNodeDetach(*this, *events_);
        Engine::OnNodeDestroy(*this);
    }

    virtual const char* GetNodeType() const override    { return "HoldNode"; }

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        bool changed = false;

        if (! events_->Events().empty())
        {
            const S& newValue = events_->Events().back();

            if (! Equals(newValue, this->value_))
            {
                changed = true;
                this->value_ = newValue;
            }
        }

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (changed)
            Engine::OnNodePulse(*this, turn);
        else
            Engine::OnNodeIdlePulse(*this, turn);
    }

    virtual int     DependencyCount() const override    { return 1; }

private:
    const std::shared_ptr<EventStreamNode<D,S>>    events_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SnapshotNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S,
    typename E
>
class SnapshotNode : public SignalNode<D,S>
{
    using Engine = typename SnapshotNode::Engine;

public:
    SnapshotNode(const std::shared_ptr<SignalNode<D,S>>& target,
                 const std::shared_ptr<EventStreamNode<D,E>>& trigger) :
        SnapshotNode::SignalNode( target->ValueRef() ),
        target_( target ),
        trigger_( trigger )
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *target_);
        Engine::OnNodeAttach(*this, *trigger_);
    }

    ~SnapshotNode()
    {
        Engine::OnNodeDetach(*this, *target_);
        Engine::OnNodeDetach(*this, *trigger_);
        Engine::OnNodeDestroy(*this);
    }

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        trigger_->SetCurrentTurn(turn);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        bool changed = false;
        
        if (! trigger_->Events().empty())
        {
            const S& newValue = target_->ValueRef();

            if (! Equals(newValue, this->value_))
            {
                changed = true;
                this->value_ = newValue;
            }
        }

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (changed)
            Engine::OnNodePulse(*this, turn);
        else
            Engine::OnNodeIdlePulse(*this, turn);
    }

    virtual const char* GetNodeType() const override        { return "SnapshotNode"; }
    virtual int         DependencyCount() const override    { return 2; }

private:
    const std::shared_ptr<SignalNode<D,S>>      target_;
    const std::shared_ptr<EventStreamNode<D,E>> trigger_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MonitorNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename E
>
class MonitorNode : public EventStreamNode<D,E>
{
    using Engine = typename MonitorNode::Engine;

public:
    MonitorNode(const std::shared_ptr<SignalNode<D,E>>& target) :
        MonitorNode::EventStreamNode( ),
        target_( target )
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *target_);
    }

    ~MonitorNode()
    {
        Engine::OnNodeDetach(*this, *target_);
        Engine::OnNodeDestroy(*this);
    }

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        this->SetCurrentTurn(turn, true);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        this->events_.push_back(target_->ValueRef());

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (! this->events_.empty())
            Engine::OnNodePulse(*this, turn);
        else
            Engine::OnNodeIdlePulse(*this, turn);
    }

    virtual const char* GetNodeType() const override        { return "MonitorNode"; }
    virtual int         DependencyCount() const override    { return 1; }

private:
    const std::shared_ptr<SignalNode<D,E>>    target_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// PulseNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S,
    typename E
>
class PulseNode : public EventStreamNode<D,S>
{
    using Engine = typename PulseNode::Engine;

public:
    PulseNode(const std::shared_ptr<SignalNode<D,S>>& target,
              const std::shared_ptr<EventStreamNode<D,E>>& trigger) :
        PulseNode::EventStreamNode( ),
        target_( target ),
        trigger_( trigger )
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *target_);
        Engine::OnNodeAttach(*this, *trigger_);
    }

    ~PulseNode()
    {
        Engine::OnNodeDetach(*this, *target_);
        Engine::OnNodeDetach(*this, *trigger_);
        Engine::OnNodeDestroy(*this);
    }

    virtual void Tick(void* turnPtr) override
    {
        typedef typename D::Engine::TurnT TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        this->SetCurrentTurn(turn, true);
        trigger_->SetCurrentTurn(turn);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        for (uint i=0; i<trigger_->Events().size(); i++)
            this->events_.push_back(target_->ValueRef());

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (! this->events_.empty())
            Engine::OnNodePulse(*this, turn);
        else
            Engine::OnNodeIdlePulse(*this, turn);
    }

    virtual const char* GetNodeType() const override        { return "PulseNode"; }
    virtual int         DependencyCount() const override    { return 2; }

private:
    const std::shared_ptr<SignalNode<D,S>>      target_;
    const std::shared_ptr<EventStreamNode<D,E>> trigger_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_GRAPH_ALGORITHMNODES_H_INCLUDED