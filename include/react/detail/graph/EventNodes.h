
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
#include <vector>

//#include "tbb/spin_mutex.h"

#include "GraphBase.h"
#include "react/common/Types.h"

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Iterators for event processing
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
class EventRange
{
public:
    using const_iterator = typename std::vector<T>::const_iterator;
    using size_type = typename std::vector<T>::size_type;

    EventRange() = delete;

    EventRange(const EventRange&) = default;
    EventRange& operator=(const EventRange&) = default;

    explicit EventRange(const std::vector<T>& data) :
        data_( data )
        { }

    const_iterator begin() const
        { return data_.begin(); }

    const_iterator end() const
        { return data_.end(); }

    size_type GetSize() const
        { return data_.size(); }

    bool IsEmpty() const
        { return data_.empty(); }

private:
    const std::vector<T>&    data_;
};

template <typename T>
using EventSink = std::back_insert_iterator<std::vector<T>>;

/******************************************/ REACT_END /******************************************/

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

    EventStreamNode(EventStreamNode&&) = default;
    EventStreamNode& operator=(EventStreamNode&&) = default;

    EventStreamNode(const EventStreamNode&) = delete;
    EventStreamNode& operator=(const EventStreamNode&) = delete;

    void SetCurrentTurn(TurnId turnId, bool forceUpdate = false, bool noClear = false)
    {
        //this->AccessBufferForClearing([&] {
        //    if (curTurnId_ != turn.Id() || forceUpdate)
        //    {
        //        curTurnId_ =  turn.Id();
        //        if (!noClear)
        //            events_.clear();
        //    }
        //});
    }

    explicit EventStreamNode(const std::shared_ptr<IReactiveGraph>& graphPtr) :
        NodeBase( graphPtr )
        { }

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
class EventSourceNode : public EventStreamNode<T>
{
public:
    EventSourceNode(const std::shared_ptr<IReactiveGraph>& graphPtr) :
        EventSourceNode::EventStreamNode( graphPtr )
        { this->RegisterMe(); }

    ~EventSourceNode()
        { this->UnregisterMe(); }

    virtual const char* GetNodeType() const override
        { return "EventSource"; }

    virtual bool IsInputNode() const override
        { return true; }

    virtual int GetDependencyCount() const override
        { return 0; }

    virtual UpdateResult Update(TurnId turnId) override
    {
        if (this->Events().size() > 0 && !changedFlag_)
        {
            this->SetCurrentTurn(turnId, true, true);
            changedFlag_ = true;
            
            return UpdateResult::changed;
        }
        else
        {
            return UpdateResult::unchanged;
        }
    }

    template <typename U>
    void EmitValue(U&& value)
    {
        // Clear input from previous turn
        if (changedFlag_)
        {
            changedFlag_ = false;
            this->Events().clear();
        }

        this->Events().push_back(std::forward<U>(value));
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
    EventMergeNode(const std::shared_ptr<IReactiveGraph>& graphPtr, const std::shared_ptr<EventStreamNode<TIns>>& ... deps) :
        EventMergeNode::EventStreamNode( graphPtr ),
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

    virtual UpdateResult Update(TurnId turnId) override
    {
        this->SetCurrentTurn(turnId, true);

        apply([this] (const auto& ... deps) { REACT_EXPAND_PACK(MergeFromDep(deps)); }, depHolder_);

        if (! this->Events().empty())
            return UpdateResult::changed;
        else
            return UpdateResult::unchanged;
    }

    virtual const char* GetNodeType() const override
        { return "EventMerge"; }

    virtual int GetDependencyCount() const override
        { return sizeof...(TIns); }

private:
    template <typename U>
    void MergeFromDep(const std::shared_ptr<EventStreamNode<U>>& other)
    {
        //arg->SetCurrentTurn(turn);
        this->Events().insert(this->Events().end(), other->Events().begin(), other->Events().end());
    }

    std::tuple<std::shared_ptr<EventStreamNode<TIns>> ...> depHolder_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventFlattenNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TOuter, typename TInner>
class EventFlattenNode : public EventStreamNode<TInner>
{
public:
    EventFlattenNode(const std::shared_ptr<IReactiveGraph>& graphPtr, const std::shared_ptr<SignalNode<TOuter>>& outer, const std::shared_ptr<EventStreamNode<TInner>>& inner) :
        EventFlattenNode::EventStreamNode( graphPtr ),
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

    virtual int GetDependencyCount() const override
        { return 2; }

    virtual UpdateResult Update(TurnId turnId) override
    {
        this->SetCurrentTurn(turnId, true);
        inner_->SetCurrentTurn(turnId);

        auto newInner = GetNodePtr(outer_->Value());

        if (newInner != inner_)
        {
            newInner->SetCurrentTurn(turnId);

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
/// EventProcessingNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TOut, typename TIn, typename F>
class EventProcessingNode : public EventStreamNode<TOut>
{
public:
    template <typename FIn>
    EventProcessingNode(const std::shared_ptr<IReactiveGraph>& graphPtr, FIn&& func, const std::shared_ptr<EventStreamNode<TIn>>& dep) :
        EventProcessingNode::EventStreamNode( graphPtr ),
        func_( std::forward<FIn>(func) ),
        dep_( dep )
    {
        this->RegisterMe();
        this->AttachToMe(dep->GetNodeId());
    }

    ~EventProcessingNode()
    {
        this->DetachFromMe(dep_->GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) override
    {
        this->SetCurrentTurn(turnId, true);

        func_(EventRange<TIn>( dep_->Events() ), std::back_inserter(this->Events()));

        if (! this->Events().empty())
            return UpdateResult::changed;
        else
            return UpdateResult::unchanged;
    }

    virtual const char* GetNodeType() const override
        { return "EventProcessing"; }

    virtual int GetDependencyCount() const override
        { return 1; }

private:
    F func_;

    std::shared_ptr<EventStreamNode<TIn>> dep_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncedEventProcessingNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TOut, typename TIn, typename F, typename ... TSyncs>
class SyncedEventProcessingNode : public EventStreamNode<TOut>
{
public:
    template <typename FIn>
    SyncedEventProcessingNode(const std::shared_ptr<IReactiveGraph>& graphPtr, FIn&& func, const std::shared_ptr<EventStreamNode<TIn>>& dep, const std::shared_ptr<SignalNode<TSyncs>>& ... syncs) :
        SyncedEventProcessingNode::EventStreamNode( graphPtr ),
        func_( std::forward<FIn>(func) ),
        dep_( dep ),
        syncHolder_( syncs ... )
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

    virtual UpdateResult Update(TurnId turnId) override
    {
        this->SetCurrentTurn(turnId, true);
        // Update of this node could be triggered from deps,
        // so make sure source doesnt contain events from last turn
        dep_->SetCurrentTurn(turnId);

        apply(
            [this] (const auto& ... syncs)
            {
                func_(EventRange<TIn>( this->dep_->Events() ), std::back_inserter(this->Events()), syncs->Value() ...);
            },
            syncHolder_);

        if (! this->Events().empty())
            return UpdateResult::changed;
        else
            return UpdateResult::unchanged;
    }

    virtual const char* GetNodeType() const override
        { return "SycnedEventProcessing"; }

    virtual int GetDependencyCount() const override
        { return 1 + sizeof...(TSyncs); }

private:
    F func_;

    std::shared_ptr<EventStreamNode<TIn>>   dep_;

    std::tuple<std::shared_ptr<SignalNode<TSyncs>>...> syncHolder_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventJoinNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename ... Ts>
class EventJoinNode : public EventStreamNode<std::tuple<Ts ...>>
{
public:
    EventJoinNode(const std::shared_ptr<IReactiveGraph>& graphPtr, const std::shared_ptr<EventStreamNode<Ts>>& ... deps) :
        EventJoinNode::EventStreamNode( graphPtr ),
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

    virtual UpdateResult Update(TurnId turnId) override
    {
        this->SetCurrentTurn(turnId, true);

        // Move events into buffers
        apply([this, turnId] (Slot<Ts>& ... slots) { REACT_EXPAND_PACK(FetchBuffer(turnId, slots)); }, slots_);

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

    virtual int GetDependencyCount() const override
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
    static void FetchBuffer(TurnId turnId, Slot<U>& slot)
    {
        slot.source->SetCurrentTurn(turnId);
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