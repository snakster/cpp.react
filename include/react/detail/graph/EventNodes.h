
//          Copyright Sebastian Jeckel 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_GRAPH_EVENTNODES_H_INCLUDED
#define REACT_DETAIL_GRAPH_EVENTNODES_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <atomic>
#include <deque>
#include <memory>
#include <utility>

#include "tbb/spin_mutex.h"

#include "GraphBase.h"
#include "react/common/Concurrency.h"
#include "react/common/Types.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward declarations
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
class SignalNode;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventStreamNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
class EventStreamNode : public NodeBase
{
public:
    using StorageType = std::vector<T>;

    EventStreamNode(IReactiveGroup* group) : NodeBase( group )
        { }

    EventStreamNode(NodeBase&&) = default;
    EventStreamNode& operator=(NodeBase&&) = default;

    EventStreamNode(const NodeBase&) = delete;
    EventStreamNode& operator=(const NodeBase&) = delete;

    void SetCurrentTurn(const TurnT& turn, bool forceUpdate = false, bool noClear = false)
    {
        this->AccessBufferForClearing([&] {
            if (curTurnId_ != turn.Id() || forceUpdate)
            {
                curTurnId_ =  turn.Id();
                if (!noClear)
                    events_.clear();
            }
        });
    }

    StorageType&  Events()
        { return events_; }

    const StorageType&  Events() const
        { return events_; }

protected:
    StorageType events_;

private:
    uint curTurnId_ { (std::numeric_limits<uint>::max)() };
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventSourceNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
class EventSourceNode : public EventStreamNode<T>, public IInputNode
{
public:
    EventSourceNode(IReactiveGroup* group) : EventSourceNode::EventStreamNode( group )
        { this->RegisterMe(); }

    ~EventSourceNode()
        { this->UnregisterMe(); }

    virtual const char* GetNodeType() const override
        { return "EventSource"; }

    virtual bool IsInputNode() const override
        { return true; }

    virtual int DependencyCount() const override
        { return 0; }

    virtual void Update(void* turnPtr) override
        { REACT_ASSERT(false, "Updated EventSourceNode\n"); }

    template <typename V>
    void AddInput(V&& v)
    {
        // Clear input from previous turn
        if (changedFlag_)
        {
            changedFlag_ = false;
            this->events_.clear();
        }

        this->events_.push_back(std::forward<V>(v));
    }

    virtual bool ApplyInput(void* turnPtr) override
    {
        if (this->events_.size() > 0 && !changedFlag_)
        {
            using TurnT = typename D::Engine::TurnT;
            TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

            this->SetCurrentTurn(turn, true, true);
            changedFlag_ = true;
            Engine::OnInputChange(*this, turn);
            return true;
        }
        else
        {
            return false;
        }
    }

private:
    bool changedFlag_ = false;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventMergeNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TOut, typename ... TIns>
class EventMergeNode : public EventStreamNode<TOut>
{
public:
    EventMergeNode(IReactiveGroup* group, const std::shared_ptr<EventStreamNode<TIns>>& ... deps) :
        EventMergeNode::EventStreamNode( group ),
        depHolder_( deps ... )
    {
        this->RegisterMe();
        REACT_EXPAND_PACK(this->AttachToMe(deps->GetNodeId()));
    }

    ~EventMergeNode()
    {
        apply([this] (const auto& ... deps) { REACT_EXPAND_PACK(this->DetachFromMe(deps->GetNodeId())); }, depHolder_);
        this->UnregisterMe();
    }

    virtual UpdateResult Update() override
    {
        // this->SetCurrentTurn(turn, true);

        apply([this] (const auto& ... deps) { REACT_EXPAND_PACK(MergeFromDep(deps)); }, depHolder_);

        if (! this->Events().empty())
            return UpdateResult::changed;
        else
            return UpdateResult::unchanged;
    }

    virtual const char* GetNodeType() const override
        { return "EventMerge"; }

    virtual int DependencyCount() const override
        { return sizeof...(Es); }

private:
    template <typename U>
    void MergeFromDep(const std::shared_ptr<EventStreamNode<U>& other)
    {
        //arg->SetCurrentTurn(turn);
        this->Events().insert(this->Events().end(), other->Events().begin(), other->Events().end());
    }

    std::tuple<std::shared_ptr<EventStreamNode<TIns> ...> depHolder_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventTransformNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TOut, typename TIn, typename F>
class EventTransformNode : public EventStreamNode<TOut>
{
public:
    template <typename U>
    EventTransformNode(IReactiveGroup* group, const std::shared_ptr<EventStreamNode<TIn>>& dep, U&& func) :
        EventTransformNode::EventStreamNode( group ),
        dep_( dep ),
        func_( std::forward<U>(func) )
    {
        this->RegisterMe();
        this->AttachToMe(dep->GetNodeId());
    }

    ~EventTransformNode()
    {
        this->DetachFromMe(dep_->GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update() override
    {
        // this->SetCurrentTurn(turn, true);

        for (const auto& v : dep_->Events())
            this->Events().push_back(func_(v));

        if (! this->Events().empty())
            return UpdateResult::changed;
        else
            return UpdateResult::unchanged;
    }

    virtual const char* GetNodeType() const override
        { return "EventTransform"; }

    virtual int DependencyCount() const override
        { return 1; }

private:
    std::shared_ptr<EventStreamNode<TIn> dep_;

    F func_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventFilterNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T, typename F>
class EventFilterNode : public EventStreamNode<T>
{
public:
    template <typename U>
    EventFilterNode(IReactiveGroup* group, const std::shared_ptr<EventStreamNode<T>>& dep, U&& pred) :
        EventFilterNode::EventStreamNode( group ),
        dep_( dep ),
        pred_( std::forward<U>(pred) )
    {
        this->RegisterMe();
        this->AttachToMe(dep->GetNodeId());
    }

    ~EventFilterNode()
    {
        this->DetachFromMe(dep_->GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update() override
    {
        // this->SetCurrentTurn(turn, true);

        for (const auto& v : dep_->Events())
            if (pred_(v))
                this->Events().push_back(v);

        if (! this->Events().empty())
            return UpdateResult::changed;
        else
            return UpdateResult::unchanged;
    }

    virtual const char* GetNodeType() const override
        { return "EventFilter"; }

    virtual int DependencyCount() const override
        { return 1; }

private:
    std::shared_ptr<EventStreamNode<T> dep_;

    F pred_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventFlattenNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TOuter, typename TInner>
class EventFlattenNode : public EventStreamNode<TInner>
{
public:
    EventFlattenNode(IReactiveGroup* group, const std::shared_ptr<SignalNode<TOuter>>& outer, const std::shared_ptr<EventStreamNode<TInner>>& inner) :
        EventFlattenNode::EventStreamNode( group ),
        outer_( outer ),
        inner_( inner )
    {
        this->RegisterMe();
        this->AttachToMe(outer->GetNodeId());
        this->AttachToMe(inner->GetNodeId());
    }

    ~EventFlattenNode()
    {
        this->DetachFromMe(inner->GetNodeId());
        this->DetachFromMe(outer->GetNodeId());
        this->UnregisterMe();
    }

    virtual const char* GetNodeType() const override
        { return "EventFlatten"; }

    virtual bool IsDynamicNode() const override
        { return true; }

    virtual int DependencyCount() const override
        { return 2; }

    virtual UpdateResult Update() override
    {
        // this->SetCurrentTurn(turn, true);
        // inner_->SetCurrentTurn(turn);

        auto newInner = GetNodePtr(outer_->ValueRef());

        if (newInner != inner_)
        {
            newInner->SetCurrentTurn(turn);

            // Topology has been changed
            auto oldInner = inner_;
            inner_ = newInner;

            this->DynamicDetachFromMe(oldInner->GetNodeId(), 0);
            this->DynamicAttachToMe(newInner->GetNodeId(), 0);

            return UpdateResult::shifted;
        }

        this->Events().insert(this->Events().end(), inner_->Events().begin(), inner_->Events().end());

        if (! this->Events().empty())
            return UpdateResult::changed;
        else
            return UpdateResult::unchanged;
    }

private:
    std::shared_ptr<SignalNode<TOuter>>       outer_;
    std::shared_ptr<EventStreamNode<TInner>>  inner_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncedEventTransformNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TOut, typename TIn, typename F, typename ... TSyncs>
class SyncedEventTransformNode : public EventStreamNode<TOut>
{
public:
    template <typename U>
    SyncedEventTransformNode(IReactiveGroup* group, const std::shared_ptr<EventStreamNode<TIn>>& dep, U&& func, const std::shared_ptr<SignalNode<TSyncs>>& ... syncs) :
        SyncedEventTransformNode::EventStreamNode( group ),
        dep_( dep ),
        func_( std::forward<U>(func) ),
        syncs_( syncs ... )
    {
        this->RegisterMe();
        this->AttachToMe(dep->GetNodeId());
        REACT_EXPAND_PACK(this->AttachToMe(syncs->GetNodeId()));
    }

    ~SyncedEventTransformNode()
    {
        apply([this] (const auto& ... syncs) { REACT_EXPAND_PACK(this->DetachFromMe(syncs->GetNodeId())); }, syncHolder_);
        this->DetachFromMe(dep_->GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update() override
    {
        //this->SetCurrentTurn(turn, true);

        // Update of this node could be triggered from deps,
        // so make sure source doesnt contain events from last turn
        // source_->SetCurrentTurn(turn);

        for (const auto& e : dep_->Events())
            this->Events().push_back(apply([this, &e] (const auto& ... syncs) { return this->func_(e, syncs->ValueRef() ...); }, syncHolder_));

        if (! this->Events().empty())
            return UpdateResult::changed;
        else
            return UpdateResult::unchanged;
    }

    virtual const char* GetNodeType() const override
        { return "SyncedEventTransform"; }

    virtual int DependencyCount() const override
        { return 1 + sizeof...(TSyncs); }

private:
    std::shared_ptr<EventStreamNode<TIn>>   dep_;

    F func_;

    std::tuple<std::shared_ptr<SignalNode<TSyncs>>...> syncHolder_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncedEventFilterNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T, typename F, typename ... TSyncs>
class SyncedEventFilterNode : public EventStreamNode<T>
{
public:
    template <typename U>
    SyncedEventFilterNode(IReactiveGroup* group, const std::shared_ptr<EventStreamNode<T>>& dep, U&& pred, const std::shared_ptr<SignalNode<TSyncs>>& ... syncs) :
        SyncedEventFilterNode::EventStreamNode( group ),
        dep_( dep ),
        pred_( std::forward<F>(pred) ),
        syncs_( syncs ... )
    {
        this->RegisterMe();
        this->AttachToMe(dep->GetNodeId());
        REACT_EXPAND_PACK(this->AttachToMe(syncs->GetNodeId()));
    }

    ~SyncedEventFilterNode()
    {
        apply([this] (const auto& ... syncs) { REACT_EXPAND_PACK(this->DetachFromMe(syncs->GetNodeId())); }, syncHolder_);
        this->DetachFromMe(dep_->GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update() override
    {
        // this->SetCurrentTurn(turn, true);

        // Update of this node could be triggered from deps,
        // so make sure source doesnt contain events from last turn
        //source_->SetCurrentTurn(turn);

        for (const auto& e : dep_->Events())
            if (apply([this, &e] (const auto& ... syncs) { return this->func_(e, syncs->ValueRef() ...); }, syncHolder_))
                this->Events().push_back(e);

        if (! this->Events().empty())
            return UpdateResult::changed;
        else
            return UpdateResult::unchanged;
    }

    virtual const char* GetNodeType() const override
        { return "SyncedEventFilter"; }

    virtual int DependencyCount() const override
        { return 1 + sizeof...(TSyncs); }

private:
    std::shared_ptr<EventStreamNode<TIn>>   dep_;

    F pred_;

    std::tuple<std::shared_ptr<SignalNode<TSyncs>>...> syncHolder_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventProcessingNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TOut, typename TIn, typename F>
class EventProcessingNode : public EventStreamNode<TOut>
{
public:
    template <typename U>
    EventProcessingNode(IReactiveGroup* group, const std::shared_ptr<EventStreamNode<TIn>>& dep, U&& func) :
        EventProcessingNode::EventStreamNode( group ),
        dep_( dep ),
        func_( std::forward<U>(func) )
    {
        this->RegisterMe();
        this->AttachToMe(dep->GetNodeId());
    }

    ~EventProcessingNode()
    {
        this->DetachFromMe(dep_->GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update() override
    {
        //this->SetCurrentTurn(turn, true);

        func_(EventRange<TIn>( source_->Events() ), std::back_inserter(this->Events()));

        if (! this->Events().empty())
            return UpdateResult::changed;
        else
            return UpdateResult::unchanged;
    }

    virtual const char* GetNodeType() const override
        { return "EventProcessing"; }

    virtual int DependencyCount() const override
        { return 1; }

private:
    std::shared_ptr<EventStreamNode<D,TIn>>   source_;

    TFunc       func_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncedEventProcessingNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TOut, typename TIn, typename F, typename ... TSyncs>
class SyncedEventProcessingNode : public EventStreamNode<TOut>
{
public:
    template <typename U>
    SyncedEventProcessingNode(IReactiveGroup* group, const std::shared_ptr<EventStreamNode<TIn>>& dep, U&& func, const std::shared_ptr<SignalNode<TSyncs>>& ... syncs) :
        SyncedEventProcessingNode::EventStreamNode( group ),
        dep_( dep ),
        func_( std::forward<U>(func) ),
        syncs_( syncs ... )
    {
        this->RegisterMe();
        this->AttachToMe(dep->GetNodeId());
        REACT_EXPAND_PACK(this->AttachToMe(syncs->GetNodeId()));
    }

    ~SyncedEventProcessingNode()
    {
        apply([this] (const auto& ... syncs) { REACT_EXPAND_PACK(this->DetachFromMe(syncs->GetNodeId())); }, syncHolder_);
        this->DetachFromMe(dep_->GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update() override
    {
        //this->SetCurrentTurn(turn, true);
        // Update of this node could be triggered from deps,
        // so make sure source doesnt contain events from last turn
        //source_->SetCurrentTurn(turn);

        apply(
            [this] (const auto& ... syncs)
            {
                func_(EventRange<TIn>( source_->Events() ), std::back_inserter(this->Events()), syncs->ValueRef() ...);
            },
            syncHolder_);

        if (! this->Events().empty())
            return UpdateResult::changed;
        else
            return UpdateResult::unchanged;
    }

    virtual const char* GetNodeType() const override
        { return "SycnedEventProcessing"; }

    virtual int DependencyCount() const override
        { return 1 + sizeof...(TSyncs); }

private:
    std::shared_ptr<EventStreamNode<TIn>>   dep_;

    F func_;

    std::tuple<std::shared_ptr<SignalNode<TSyncs>>...> syncHolder_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventJoinNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename ... Ts>
class EventJoinNode : public EventStreamNode<std::tuple<Ts ...>>
{
public:
    EventJoinNode(IReactiveGroup* group, const std::shared_ptr<EventStreamNode<Ts>>& ... deps) :
        EventJoinNode::EventStreamNode( group ),
        slots_( deps ... )
    {
        this->RegisterMe();
        REACT_EXPAND_PACK(this->AttachToMe(deps->GetNodeId()));
    }

    ~EventJoinNode()
    {
        apply([this] (const auto& ... slots) { REACT_EXPAND_PACK(this->DetachFromMe(slots.source->GetNodeId())); }, slots_);
        this->UnregisterMe();
    }

    virtual UpdateResult Update() override
    {
        //this->SetCurrentTurn(turn, true);

        // Move events into buffers
        apply([this, &turn] (Slot<Ts>& ... slots) { REACT_EXPAND_PACK(FetchBuffer(turn, slots)); }, slots_);

        while (true)
        {
            bool isReady = true;

            // All slots ready?
            apply(
                [this,&isReady] (Slot<Ts>& ... slots) {
                    // Todo: combine return values instead
                    REACT_EXPAND_PACK(CheckSlot(slots, isReady));
                },
                slots_);

            if (!isReady)
                break;

            // Pop values from buffers and emit tuple
            apply(
                [this] (Slot<Ts>& ... slots)
                {
                    this->Events().emplace_back(slots.buffer.front() ...);
                    REACT_EXPAND_PACK(slots.buffer.pop_front());
                },
                slots_);
        }

        if (! this->Events().empty())
            return UpdateResult::changed;
        else
            return UpdateResult::unchanged;
    }

    virtual const char* GetNodeType() const override
        { return "EventJoin"; }

    virtual int DependencyCount() const override
        { return sizeof...(Ts); }

private:
    template <typename U>
    struct Slot
    {
        Slot(const std::shared_ptr<EventStreamNode<U>>& src) :
            source( src )
            { }

        std::shared_ptr<EventStreamNode<U>>     source;
        std::deque<U>                           buffer;
    };

    template <typename U>
    static void FetchBuffer(TurnT& turn, Slot<U>& slot)
    {
        //slot.Source->SetCurrentTurn(turn);
        slot.buffer.insert(slot.buffer.end(), slot.source->Events().begin(), slot.source->Events().end());
    }

    template <typename T>
    static void CheckSlot(Slot<T>& slot, bool& isReady)
    {
        bool t = isReady && !slot.buffer.empty();
        isReady = t;
    }

    std::tuple<Slot<Ts>...> slots_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_GRAPH_EVENTNODES_H_INCLUDED