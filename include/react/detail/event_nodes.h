
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
#include "react/common/utility.h"

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Iterators for event processing
///////////////////////////////////////////////////////////////////////////////////////////////////

/******************************************/ REACT_END /******************************************/

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward declarations
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class StateNode;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E>
class EventNode : public NodeBase
{
public:
    explicit EventNode(const Group& group) :
        EventNode::NodeBase( group )
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
class EventSourceNode : public EventNode<E>
{
public:
    EventSourceNode(const Group& group) :
        EventSourceNode::EventNode( group )
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
class EventMergeNode : public EventNode<E>
{
public:
    EventMergeNode(const Group& group, const Event<TInputs>& ... deps) :
        EventMergeNode::EventNode( group ),
        inputs_( deps ... )
    {
        this->RegisterMe();
        REACT_EXPAND_PACK(this->AttachToMe(GetInternals(deps).GetNodeId()));
    }

    ~EventMergeNode()
    {
        apply([this] (const auto& ... inputs)
            { REACT_EXPAND_PACK(this->DetachFromMe(GetInternals(inputs).GetNodeId())); }, inputs_);
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        apply([this] (auto& ... inputs)
            { REACT_EXPAND_PACK(MergeFromInput(inputs)); }, inputs_);

        if (! this->Events().empty())
            return UpdateResult::changed;
        else
            return UpdateResult::unchanged;
    }

private:
    template <typename U>
    void MergeFromInput(Event<U>& dep)
    {
        auto& depInternals = GetInternals(dep);
        this->Events().insert(this->Events().end(), depInternals.Events().begin(), depInternals.Events().end());
    }

    std::tuple<Event<TInputs> ...> inputs_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventSlotNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E>
class EventSlotNode : public EventNode<E>
{
public:
    EventSlotNode(const Group& group) :
        EventSlotNode::EventNode( group )
    {
        inputNodeId_ = GetGraphPtr()->RegisterNode(&slotInput_, NodeCategory::dyninput);
        this->RegisterMe();

        this->AttachToMe(inputNodeId_);
    }

    ~EventSlotNode()
    {
        RemoveAllSlotInputs();
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

    void AddSlotInput(const Event<E>& input)
    {
        auto it = std::find(inputs_.begin(), inputs_.end(), input);
        if (it == inputs_.end())
        {
            inputs_.push_back(input);
            this->AttachToMe(GetInternals(input).GetNodeId());
        }
    }

    void RemoveSlotInput(const Event<E>& input)
    {
        auto it = std::find(inputs_.begin(), inputs_.end(), input);
        if (it != inputs_.end())
        {
            inputs_.erase(it);
            this->DetachFromMe(GetInternals(input).GetNodeId());
        }
    }

    void RemoveAllSlotInputs()
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
class EventProcessingNode : public EventNode<TOut>
{
public:
    template <typename FIn>
    EventProcessingNode(const Group& group, FIn&& func, const Event<TIn>& dep) :
        EventProcessingNode::EventNode( group ),
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
        func_(GetInternals(dep_).Events(), std::back_inserter(this->Events()));

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
class SyncedEventProcessingNode : public EventNode<TOut>
{
public:
    template <typename FIn>
    SyncedEventProcessingNode(const Group& group, FIn&& func, const Event<TIn>& dep, const State<TSyncs>& ... syncs) :
        SyncedEventProcessingNode::EventNode( group ),
        func_( std::forward<FIn>(func) ),
        dep_( dep ),
        syncHolder_( syncs ... )
    {
        this->RegisterMe();
        this->AttachToMe(GetInternals(dep).GetNodeId());
        REACT_EXPAND_PACK(this->AttachToMe(GetInternals(syncs).GetNodeId()));
    }

    ~SyncedEventProcessingNode()
    {
        apply([this] (const auto& ... syncs)
            { REACT_EXPAND_PACK(this->DetachFromMe(GetInternals(syncs).GetNodeId())); }, syncHolder_);
        this->DetachFromMe(GetInternals(dep_).GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        // Updates might be triggered even if only sync nodes changed. Ignore those.
        if (GetInternals(dep_).Events().empty())
            return UpdateResult::unchanged;

        apply([this] (const auto& ... syncs)
            {
                func_(GetInternals(dep_).Events(), std::back_inserter(this->Events()), GetInternals(syncs).Value() ...);
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

    std::tuple<State<TSyncs> ...> syncHolder_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventJoinNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename ... Ts>
class EventJoinNode : public EventNode<std::tuple<Ts ...>>
{
public:
    EventJoinNode(const Group& group, const Event<Ts>& ... deps) :
        EventJoinNode::EventNode( group ),
        slots_( deps ... )
    {
        this->RegisterMe();
        REACT_EXPAND_PACK(this->AttachToMe(GetInternals(deps).GetNodeId()));
    }

    ~EventJoinNode()
    {
        apply([this] (const auto& ... slots)
            { REACT_EXPAND_PACK(this->DetachFromMe(GetInternals(slots.source).GetNodeId())); }, slots_);
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        // Move events into buffers.
        apply([this, turnId] (Slot<Ts>& ... slots)
            { REACT_EXPAND_PACK(FetchBuffer(turnId, slots)); }, slots_);

        while (true)
        {
            bool isReady = true;

            // All slots ready?
            apply([this, &isReady] (Slot<Ts>& ... slots)
                {
                    // Todo: combine return values instead
                    REACT_EXPAND_PACK(CheckSlot(slots, isReady));
                },
                slots_);

            if (!isReady)
                break;

            // Pop values from buffers and emit tuple.
            apply([this] (Slot<Ts>& ... slots)
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
class EventLinkNode : public EventNode<E>
{
public:
    EventLinkNode(const Group& group, const Event<E>& dep) :
        EventLinkNode::EventNode( group ),
        dep_( dep ),
        srcGroup_( dep.GetGroup() )
    {
        this->RegisterMe(NodeCategory::input);

        auto& srcGraphPtr = GetInternals(srcGroup_).GetGraphPtr();
        outputNodeId_ = srcGraphPtr->RegisterNode(&linkOutput_, NodeCategory::linkoutput);
        
        srcGraphPtr->AttachNode(outputNodeId_, GetInternals(dep).GetNodeId());
    }

    ~EventLinkNode()
    {
        auto& srcGraphPtr = GetInternals(srcGroup_).GetGraphPtr();
        srcGraphPtr->DetachNode(outputNodeId_, GetInternals(dep_).GetNodeId());
        srcGraphPtr->UnregisterNode(outputNodeId_);

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
        virtual UpdateResult Update(TurnId turnId) noexcept override
            { return UpdateResult::changed; }

        virtual void CollectOutput(LinkOutputMap& output) override
        {
            if (auto p = parent.lock())
            {
                auto* rawPtr = p->GetGraphPtr().get();
                output[rawPtr].push_back(
                    [storedParent = std::move(p), storedEvents = GetInternals(p->dep_).Events()] () mutable
                    {
                        NodeId nodeId = storedParent->GetNodeId();
                        auto& graphPtr = storedParent->GetGraphPtr();

                        graphPtr->PushInput(nodeId,
                            [&storedParent, &storedEvents]
                            {
                                storedParent->SetEvents(std::move(storedEvents));
                            });
                    });
            }
        }

        std::weak_ptr<EventLinkNode> parent;  
    };

    Event<E>    dep_;
    Group       srcGroup_;
    NodeId      outputNodeId_;

    VirtualOutputNode linkOutput_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventInternals
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E>
class EventInternals
{
public:
    EventInternals() = default;

    EventInternals(const EventInternals&) = default;
    EventInternals& operator=(const EventInternals&) = default;

    EventInternals(EventInternals&&) = default;
    EventInternals& operator=(EventInternals&&) = default;

    explicit EventInternals(std::shared_ptr<EventNode<E>>&& nodePtr) :
        nodePtr_( std::move(nodePtr) )
    { }

    auto GetNodePtr() -> std::shared_ptr<EventNode<E>>&
        { return nodePtr_; }

    auto GetNodePtr() const -> const std::shared_ptr<EventNode<E>>&
        { return nodePtr_; }

    NodeId GetNodeId() const
        { return nodePtr_->GetNodeId(); }

    EventValueList<E>& Events()
        { return nodePtr_->Events(); }

    const EventValueList<E>& Events() const
        { return nodePtr_->Events(); }

private:
    std::shared_ptr<EventNode<E>> nodePtr_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_EVENT_NODES_H_INCLUDED