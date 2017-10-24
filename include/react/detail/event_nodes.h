
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_EVENT_NODES_H_INCLUDED
#define REACT_DETAIL_EVENT_NODES_H_INCLUDED

#pragma once

#include "react/detail/defs.h"

#include <atomic>
#include <deque>
#include <memory>
#include <utility>
#include <vector>

//#include "tbb/spin_mutex.h"

#include "node_base.h"
#include "react/common/Types.h"

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Iterators for event processing
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E = Token>
using EventValueList = std::vector<E>;

template <typename E>
using EventValueSink = std::back_insert_iterator<std::vector<E>>;

/******************************************/ REACT_END /******************************************/

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward declarations
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class SignalNode;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventStreamNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E>
class EventStreamNode : public NodeBase
{
public:
    EventStreamNode(EventStreamNode&&) = default;
    EventStreamNode& operator=(EventStreamNode&&) = default;

    EventStreamNode(const EventStreamNode&) = delete;
    EventStreamNode& operator=(const EventStreamNode&) = delete;

    explicit EventStreamNode(const Group& group) :
        EventStreamNode::NodeBase( group )
    { }

    EventValueList<E>& Events()
        { return events_; }

    const EventValueList<E>& Events() const
        { return events_; }

    virtual void Clear() noexcept override
        { events_.clear(); }

private:
    EventValueList<E> events_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventSourceNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E>
class EventSourceNode : public EventStreamNode<E>
{
public:
    EventSourceNode(const Group& group) :
        EventSourceNode::EventStreamNode( group )
    {
        this->RegisterMe(NodeCategory::input);
    }

    ~EventSourceNode()
    {
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        if (! this->Events().empty())
            return UpdateResult::changed;
        else
            return UpdateResult::unchanged;
    }

    template <typename U>
    void EmitValue(U&& value)
        { this->Events().push_back(std::forward<U>(value)); }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventMergeNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E, typename ... TInputs>
class EventMergeNode : public EventStreamNode<E>
{
public:
    EventMergeNode(const Group& group, const Event<TInputs>& ... deps) :
        EventMergeNode::EventStreamNode( group ),
        inputs_( deps ... )
    {
        this->RegisterMe();
        REACT_EXPAND_PACK(this->AttachToMe(GetInternals(deps).GetNodeId()));
    }

    ~EventMergeNode()
    {
        std::apply([this] (const auto& ... deps)
            { REACT_EXPAND_PACK(this->DetachFromMe(GetInternals(deps).GetNodeId())); }, depHolder_);
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        std::apply([this] (auto& ... deps)
            { REACT_EXPAND_PACK(MergeFromDep(deps)); }, depHolder_);

        if (! this->Events().empty())
            return UpdateResult::changed;
        else
            return UpdateResult::unchanged;
    }

private:
    template <typename U>
    void MergeFromInput(Event<U>& dep)
    {
        EventInternals& depInternals = GetInternals(dep);
        this->Events().insert(this->Events().end(), depInternals.Events().begin(), depInternals.Events().end());
    }

    std::tuple<Event<TInputs> ...> inputs_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventSlotNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E>
class EventSlotNode : public EventStreamNode<E>
{
public:
    EventSlotNode(const Group& group) :
        EventSlotNode::EventStreamNode( group )
    {
        inputNodeId_ = GetGraphPtr()->RegisterNode(&slotInput_, NodeCategory::dyninput);
        this->RegisterMe();

        this->AttachToMe(inputNodeId_);
    }

    ~EventSlotNode()
    {
        RemoveAllInputs();
        this->DetachFromMe(inputNodeId_);

        this->UnregisterMe();
        GetGraphPtr()->UnregisterNode(inputNodeId_);
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        for (auto& e : inputs_)
        {
            const auto& events = GetInternals(e).Events();
            this->Events().insert(this->Events().end(), events.begin(), events.end());
        }

        if (! this->Events().empty())
            return UpdateResult::changed;
        else
            return UpdateResult::unchanged;
    }

    void AddInput(const Event<E>& input)
    {
        auto it = std::find(inputs_.begin(), inputs_.end(), input);
        if (it == inputs_.end())
        {
            inputs_.push_back(input);
            this->AttachToMe(GetInternals(input).GetNodeId());
        }
    }

    void RemoveInput(const Event<E>& input)
    {
        auto it = std::find(inputs_.begin(), inputs_.end(), input);
        if (it != inputs_.end())
        {
            inputs_.erase(it);
            this->DetachFromMe(GetInternals(input).GetNodeId());
        }
    }

    void RemoveAllInputs()
    {
        for (const auto& e : inputs_)
            this->DetachFromMe(GetInternals(e).GetNodeId());

        inputs_.clear();
    }

    NodeId GetInputNodeId() const
        { return inputNodeId_; }

private:        
    struct VirtualInputNode : public IReactNode
    {
        virtual UpdateResult Update(TurnId turnId) noexcept override
            { return UpdateResult::changed; }
    };

    std::vector<Event<E>> inputs_;

    NodeId              inputNodeId_;
    VirtualInputNode    slotInput_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventProcessingNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TOut, typename TIn, typename F>
class EventProcessingNode : public EventStreamNode<TOut>
{
public:
    template <typename FIn>
    EventProcessingNode(const Group& group, FIn&& func, const Event<TIn>& dep) :
        EventProcessingNode::EventStreamNode( group ),
        func_( std::forward<FIn>(func) ),
        dep_( dep )
    {
        this->RegisterMe();
        this->AttachToMe(GetInternals(dep).GetNodeId());
    }

    ~EventProcessingNode()
    {
        this->DetachFromMe(GetInternals(dep_).GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        func_(EventRange<TIn>( GetInternals(dep_).Events() ), std::back_inserter(this->Events()));

        if (! this->Events().empty())
            return UpdateResult::changed;
        else
            return UpdateResult::unchanged;
    }

private:
    F func_;

    Event<TIn> dep_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncedEventProcessingNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TOut, typename TIn, typename F, typename ... TSyncs>
class SyncedEventProcessingNode : public EventStreamNode<TOut>
{
public:
    template <typename FIn>
    SyncedEventProcessingNode(const Group& group, FIn&& func, const Event<TIn>& dep, const Signal<TSyncs>& ... syncs) :
        SyncedEventProcessingNode::EventStreamNode( group ),
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
        std::apply([this] (const auto& ... syncs) { REACT_EXPAND_PACK(this->DetachFromMe(syncs->GetNodeId())); }, syncHolder_);
        this->DetachFromMe(dep_->GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        // Updates might be triggered even if only sync nodes changed. Ignore those.
        if (dep_->Events().empty())
            return UpdateResult::unchanged;

        std::apply(
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

private:
    F func_;

    Event<TIn> dep_;

    std::tuple<Signal<TSyncs> ...> syncHolder_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventJoinNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename ... Ts>
class EventJoinNode : public EventStreamNode<std::tuple<Ts ...>>
{
public:
    EventJoinNode(const Group& group, const Event<Ts>& ... deps) :
        EventJoinNode::EventStreamNode( group ),
        slots_( deps ... )
    {
        this->RegisterMe();
        REACT_EXPAND_PACK(this->AttachToMe(GetInternals(deps).GetNodeId()));
    }

    ~EventJoinNode()
    {
        std::apply([this] (const auto& ... slots) { REACT_EXPAND_PACK(this->DetachFromMe(GetInternals(slots.source).GetNodeId())); }, slots_);
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        // Move events into buffers.
        std::apply([this, turnId] (Slot<Ts>& ... slots) { REACT_EXPAND_PACK(FetchBuffer(turnId, slots)); }, slots_);

        while (true)
        {
            bool isReady = true;

            // All slots ready?
            std::apply(
                [this, &isReady] (Slot<Ts>& ... slots)
                {
                    // Todo: combine return values instead
                    REACT_EXPAND_PACK(CheckSlot(slots, isReady));
                },
                slots_);

            if (!isReady)
                break;

            // Pop values from buffers and emit tuple.
            std::apply(
                [this] (Slot<Ts>& ... slots)
                {
                    this->Events().emplace_back(slots.buffer.front() ...);
                    REACT_EXPAND_PACK(slots.buffer.pop_front());
                },
                slots_);
        }

        if (! this->Events().empty())
        {
            this->SetPendingSuccessorCount(successorCount);
            return UpdateResult::changed;
        }
        else
        {
            return UpdateResult::unchanged;
        }
    }

private:
    template <typename U>
    struct Slot
    {
        Slot(const Event<U>& src) :
            source( src )
            { }

        Event<U>        source;
        std::deque<U>   buffer;
    };

    template <typename U>
    static void FetchBuffer(TurnId turnId, Slot<U>& slot)
    {
        slot.buffer.insert(slot.buffer.end(), GetInternals(slot.source).Events().begin(), GetInternals(slot.source).Events().end());
        GetInternals(slot.source).DecrementPendingSuccessorCount();
    }

    template <typename T>
    static void CheckSlot(Slot<T>& slot, bool& isReady)
    {
        bool t = isReady && !slot.buffer.empty();
        isReady = t;
    }

    std::tuple<Slot<Ts>...> slots_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventLinkNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E>
class EventLinkNode : public EventStreamNode<E>
{
public:
    EventLinkNode(const Group& group, const Event<E>& dep) :
        EventLinkNode::EventStreamNode( group ),
        linkOutput_( dep )
    {
        this->RegisterMe(NodeCategory::input);
    }

    ~EventLinkNode()
    {
        auto& linkCache = GetGraphPtr()->GetLinkCache();
        linkCache.Erase(this);

        this->UnregisterMe();
    }

    void SetWeakSelfPtr(const std::weak_ptr<EventLinkNode>& self)
        { linkOutput_.parent = self; }

    virtual UpdateResult Update(TurnId turnId) noexcept override
        { return UpdateResult::changed; }

    void SetEvents(EventValueList<E>&& events)
        { this->Events() = std::move(events); }

private:
    struct VirtualOutputNode : public IReactNode
    {
        VirtualOutputNode(const Event<E>& depIn) :
            parent( ),
            dep( depIn ),
            srcGroup( depIn.GetGroup() )
        {
            auto& srcGraphPtr = GetInternals(srcGroup).GetGraphPtr();
            nodeId = srcGraphPtr->RegisterNode(this, NodeCategory::linkoutput);
            srcGraphPtr->AttachNode(nodeId, GetInternals(dep).GetNodeId());
        }

        ~VirtualOutputNode()
        {
            auto& srcGraphPtr = GetInternals(srcGroup).GetGraphPtr();
            srcGraphPtr->DetachNode(nodeId, GetInternals(dep).GetNodeId());
            srcGraphPtr->UnregisterNode(nodeId);
        }

        virtual UpdateResult Update(TurnId turnId) noexcept override
            { return UpdateResult::changed; }

        virtual void CollectOutput(LinkOutputMap& output) override
        {
            if (auto p = parent.lock())
            {
                auto* rawPtr = p->GetGraphPtr().get();
                output[rawPtr].push_back(
                    [storedParent = std::move(p), storedEvents = GetInternals(dep).Events()] () mutable
                    {
                        NodeId nodeId = storedParent->GetNodeId();
                        auto& graphPtr = storedParent->GetGraphPtr();

                        graphPtr->AddInput(nodeId,
                            [&storedParent, &storedEvents]
                            {
                                storedParent->SetEvents(std::move(storedEvents));
                            });
                    });
            }
        }

        std::weak_ptr<EventLinkNode> parent;

        NodeId      nodeId;
        Event<E>    dep;
        Group       srcGroup;
    };

    VirtualOutputNode linkOutput_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventInternals
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E>
class EventInternals
{
public:
    EventInternals(const EventInternals&) = default;
    EventInternals& operator=(const EventInternals&) = default;

    EventInternals(EventInternals&&) = default;
    EventInternals& operator=(EventInternals&&) = default;

    explicit EventInternals(std::shared_ptr<EventStreamNode<E>>&& nodePtr) :
        nodePtr_( std::move(nodePtr) )
    { }

    auto GetNodePtr() -> std::shared_ptr<EventStreamNode<E>>&
        { return nodePtr_; }

    auto GetNodePtr() const -> const std::shared_ptr<EventStreamNode<E>>&
        { return nodePtr_; }

    NodeId GetNodeId() const
        { return nodePtr_->GetNodeId(); }

    EventValueList<E>& Events()
        { return nodePtr_->Events(); }

    const EventValueList<E>& Events() const
        { return nodePtr_->Events(); }

private:
    std::shared_ptr<EventStreamNode<E>> nodePtr_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_EVENT_NODES_H_INCLUDED