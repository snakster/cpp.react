
//          Copyright Sebastian Jeckel 2016.
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
template <typename S, typename F, typename E>
class IterateNode : public SignalNode<S>
{
public:
    template <typename T, typename FIn>
    IterateNode(const std::shared_ptr<IReactiveGraph>& graphPtr, T&& init, FIn&& func, const std::shared_ptr<EventStreamNode<E>>& events) :
        IterateNode::SignalNode( graphPtr, std::forward<T>(init) ),
        func_( std::forward<FIn>(func) ),
        events_( events )
    {
        this->RegisterMe();
        this->AttachToMe(events->GetNodeId());
    }

    ~IterateNode()
    {
        this->DetachFromMe(events_->GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) override
    {
        S newValue = func_(EventRange<E>( events_->Events() ), this->Value());

        if (! (newValue == this->Value()))
        {
            this->Value() = std::move(newValue);
            return UpdateResult::changed;
        }
        else
        {
            return UpdateResult::unchanged;
        }
    }

    virtual const char* GetNodeType() const override
        { return "Iterate"; }

    virtual int GetDependencyCount() const override
        { return 1; }

private:
    std::shared_ptr<EventStreamNode<E>> events_;
    
    F func_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IterateByRefNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename F, typename E>
class IterateByRefNode : public SignalNode<S>
{
public:
    template <typename T, typename FIn>
    IterateByRefNode(const std::shared_ptr<IReactiveGraph>& graphPtr, T&& init, FIn&& func, const std::shared_ptr<EventStreamNode<E>>& events) :
        IterateByRefNode::SignalNode( graphPtr, std::forward<T>(init) ),
        func_( std::forward<FIn>(func) ),
        events_( events )
    {
        this->RegisterMe();
        this->AttachToMe(events->GetNodeId());
    }

    ~IterateByRefNode()
    {
        this->DetachFromMe(events_->GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) override
    {
        func_(EventRange<E>( events_->Events() ), this->Value());

        // Always assume change
        return UpdateResult::changed;
    }

    virtual const char* GetNodeType() const override
        { return "IterateByRefNode"; }

    virtual int GetDependencyCount() const override
        { return 1; }

protected:
    F   func_;

    std::shared_ptr<EventStreamNode<E>> events_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncedIterateNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename F, typename E, typename ... TSyncs>
class SyncedIterateNode : public SignalNode<S>
{
public:
    template <typename T, typename FIn>
    SyncedIterateNode(const std::shared_ptr<IReactiveGraph>& graphPtr, T&& init, FIn&& func, const std::shared_ptr<EventStreamNode<E>>& events, const std::shared_ptr<SignalNode<TSyncs>>& ... syncs) :
        SyncedIterateNode::SignalNode( graphPtr, std::forward<T>(init) ),
        func_( std::forward<FIn>(func) ),
        events_( events ),
        syncHolder_( syncs ... )
    {
        this->RegisterMe();
        this->AttachToMe(events->GetNodeId());
        REACT_EXPAND_PACK(this->AttachToMe(syncs->GetNodeId()));
    }

    ~SyncedIterateNode()
    {
        apply([this] (const auto& ... syncs) { REACT_EXPAND_PACK(this->DetachFromMe(syncs->GetNodeId())); }, syncHolder_);
        this->DetachFromMe(events_->GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) override
    {
        S newValue = apply(
            [this] (const auto& ... syncs)
            {
                return func_(EventRange<E>( events_->Events() ), this->Value(), syncs->Value() ...);
            },
            syncHolder_);

        if (! (newValue == this->Value()))
        {
            this->Value() = std::move(newValue);
            return UpdateResult::changed;
        }
        else
        {
            return UpdateResult::unchanged;
        }   
    }

    virtual const char* GetNodeType() const override
        { return "SyncedIterate"; }

    virtual int GetDependencyCount() const override
        { return 1 + sizeof...(TSyncs); }

private:
    F func_;

    std::shared_ptr<EventStreamNode<E>> events_;

    std::tuple<std::shared_ptr<SignalNode<TSyncs>>...> syncHolder_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncedIterateByRefNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename F, typename E, typename ... TSyncs>
class SyncedIterateByRefNode : public SignalNode<S>
{
public:
    template <typename T, typename FIn>
    SyncedIterateByRefNode(const std::shared_ptr<IReactiveGraph>& graphPtr, T&& init, FIn&& func, const std::shared_ptr<EventStreamNode<E>>& events, const std::shared_ptr<SignalNode<TSyncs>>& ... syncs) :
        SyncedIterateByRefNode::SignalNode( graphPtr, std::forward<T>(init) ),
        func_( std::forward<FIn>(func) ),
        events_( events ),
        syncHolder_( syncs ... )
    {
        this->RegisterMe();
        this->AttachToMe(events->GetNodeId());
        REACT_EXPAND_PACK(this->AttachToMe(syncs->GetNodeId()));
    }

    ~SyncedIterateByRefNode()
    {
        apply([this] (const auto& ... syncs) { REACT_EXPAND_PACK(this->DetachFromMe(syncs->GetNodeId())); }, syncHolder_);
        this->DetachFromMe(events_->GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) override
    {
        events_->SetCurrentTurn(turnId);

        bool changed = false;

        if (! events_->Events().empty())
        {
            apply(
                [this] (const auto& ... args)
                {
                    func_(EventRange<E>( events_->Events() ), this->Value(), args->Value() ...);
                },
                syncHolder_);

            return UpdateResult::changed;
        }
        else
        {
            return UpdateResult::unchanged;
        }
    }

    virtual const char* GetNodeType() const override
        { return "SyncedIterateByRef"; }

    virtual int GetDependencyCount() const override
        { return 1 + sizeof...(TSyncs); }

private:
    F func_;

    std::shared_ptr<EventStreamNode<E>> events_;

    std::tuple<std::shared_ptr<SignalNode<TSyncs>>...> syncHolder_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// HoldNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class HoldNode : public SignalNode<S>
{
public:
    template <typename T>
    HoldNode(const std::shared_ptr<IReactiveGraph>& graphPtr, T&& init, const std::shared_ptr<EventStreamNode<S>>& events) :
        HoldNode::SignalNode( graphPtr, std::forward<T>(init) ),
        events_( events )
    {
        this->RegisterMe();
        this->AttachToMe(events->GetNodeId());
    }

    ~HoldNode()
    {
        this->DetachFromMe(events_->GetNodeId());
        this->UnregisterMe();
    }

    virtual const char* GetNodeType() const override
        { return "HoldNode"; }

    virtual UpdateResult Update(TurnId turnId) override
    {
        bool changed = false;

        if (! events_->Events().empty())
        {
            const S& newValue = events_->Events().back();

            if (! (newValue == this->Value()))
            {
                changed = true;
                this->Value() = newValue;
            }
        }

        if (changed)
            return UpdateResult::changed;
        else
            return UpdateResult::unchanged;
    }

    virtual int GetDependencyCount() const override
        { return 1; }

private:
    const std::shared_ptr<EventStreamNode<S>>    events_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SnapshotNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename E>
class SnapshotNode : public SignalNode<S>
{
public:
    SnapshotNode(const std::shared_ptr<IReactiveGraph>& graphPtr, const std::shared_ptr<SignalNode<S>>& target, const std::shared_ptr<EventStreamNode<E>>& trigger) :
        SnapshotNode::SignalNode( graphPtr, target->Value() ),
        target_( target ),
        trigger_( trigger )
    {
        this->RegisterMe();
        this->AttachToMe(target->GetNodeId());
        this->AttachToMe(trigger->GetNodeId());
    }

    ~SnapshotNode()
    {
        this->DetachFromMe(trigger_->GetNodeId());
        this->DetachFromMe(target_->GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) override
    {
        trigger_->SetCurrentTurn(turnId);

        bool changed = false;
        
        if (! trigger_->Events().empty())
        {
            const S& newValue = target_->Value();

            if (! Equals(newValue, this->Value()))
            {
                changed = true;
                this->Value() = newValue;
            }
        }

        if (changed)
            return UpdateResult::changed;
        else
            return UpdateResult::unchanged;
    }

    virtual const char* GetNodeType() const override
        { return "Snapshot"; }

    virtual int GetDependencyCount() const override
        { return 2; }

private:
    std::shared_ptr<SignalNode<S>>      target_;
    std::shared_ptr<EventStreamNode<E>> trigger_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MonitorNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class MonitorNode : public EventStreamNode<S>
{
public:
    MonitorNode(const std::shared_ptr<IReactiveGraph>& graphPtr, const std::shared_ptr<SignalNode<S>>& target) :
        MonitorNode::EventStreamNode( graphPtr ),
        target_( target )
    {
        this->RegisterMe(NodeFlags::buffered);
        this->AttachToMe(target->GetNodeId());
    }

    ~MonitorNode()
    {
        this->DetachFromMe(target_->GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) override
    {
        this->Events().push_back(target_->Value());
        return UpdateResult::changed;
    }

    virtual const char* GetNodeType() const override
        { return "Monitor"; }

    virtual int GetDependencyCount() const override
        { return 1; }

private:
    std::shared_ptr<SignalNode<S>>    target_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// PulseNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename E>
class PulseNode : public EventStreamNode<S>
{
public:
    PulseNode(const std::shared_ptr<IReactiveGraph>& graphPtr, const std::shared_ptr<SignalNode<S>>& target, const std::shared_ptr<EventStreamNode<E>>& trigger) :
        PulseNode::EventStreamNode( graphPtr ),
        target_( target ),
        trigger_( trigger )
    {
        this->RegisterMe(NodeFlags::buffered);
        this->AttachToMe(target->GetNodeId());
        this->AttachToMe(trigger->GetNodeId());
    }

    ~PulseNode()
    {
        this->DetachFromMe(trigger_->GetNodeId());
        this->DetachFromMe(target_->GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) override
    {
        for (size_t i=0; i<trigger_->Events().size(); i++)
            this->Events().push_back(target_->Value());

        if (! this->Events().empty())
            return UpdateResult::changed;
        else
            return UpdateResult::unchanged;
    }

    virtual const char* GetNodeType() const override
        { return "PulseNode"; }

    virtual int GetDependencyCount() const override
        { return 2; }

private:
    const std::shared_ptr<SignalNode<S>>      target_;
    const std::shared_ptr<EventStreamNode<E>> trigger_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_GRAPH_ALGORITHMNODES_H_INCLUDED