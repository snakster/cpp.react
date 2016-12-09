
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
template <typename E = Token>
class EventRange
{
public:
    using const_iterator = typename std::vector<E>::const_iterator;
    using size_type = typename std::vector<E>::size_type;

    EventRange() = delete;

    EventRange(const EventRange&) = default;
    EventRange& operator=(const EventRange&) = default;

    explicit EventRange(const std::vector<E>& data) :
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
    const std::vector<E>&    data_;
};

template <typename E>
using EventSink = std::back_insert_iterator<std::vector<E>>;

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
    using StorageType = std::vector<E>;

    EventStreamNode(EventStreamNode&&) = default;
    EventStreamNode& operator=(EventStreamNode&&) = default;

    EventStreamNode(const EventStreamNode&) = delete;
    EventStreamNode& operator=(const EventStreamNode&) = delete;

    explicit EventStreamNode(const std::shared_ptr<ReactiveGraph>& graphPtr) :
        NodeBase( graphPtr )
        { }

    StorageType& Events()
        { return events_; }

    const StorageType& Events() const
        { return events_; }


    void SetPendingSuccessorCount(int count)
    {
        if (count == 0)
        {
            // If there are no successors, buffer is cleared immediately.
            events_.clear();
        }
        else
        {
            // Otherwise, the last finished successor clears it.
            pendingSuccessorCount_ = count;
        }
    }

    void DecrementPendingSuccessorCount()
    {
        // Not all predecessors of a node might be visited during a turn.
        // In this case, the count is zero and the call to this function should be ignored.
        if (pendingSuccessorCount_ == 0)
            return;

        // Last successor to arrive clears the buffer.
        if (pendingSuccessorCount_-- == 1)
            events_.clear();
    };

private:
    StorageType events_;
    
    std::atomic<int> pendingSuccessorCount_ = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventSourceNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
class EventSourceNode : public EventStreamNode<T>
{
public:
    EventSourceNode(const std::shared_ptr<ReactiveGraph>& graphPtr) :
        EventSourceNode::EventStreamNode( graphPtr )
        { this->RegisterMe(NodeCategory::input); }

    ~EventSourceNode()
        { this->UnregisterMe(); }

    virtual const char* GetNodeType() const override
        { return "EventSource"; }

    virtual int GetDependencyCount() const override
        { return 0; }

    virtual UpdateResult Update(TurnId turnId, int successorCount) override
    {
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

    template <typename U>
    void EmitValue(U&& value)
        { this->Events().push_back(std::forward<U>(value)); }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventMergeNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TOut, typename ... TIns>
class EventMergeNode : public EventStreamNode<TOut>
{
public:
    EventMergeNode(const std::shared_ptr<ReactiveGraph>& graphPtr, const std::shared_ptr<EventStreamNode<TIns>>& ... deps) :
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

    virtual UpdateResult Update(TurnId turnId, int successorCount) override
    {
        apply([this] (const auto& ... deps) { REACT_EXPAND_PACK(MergeFromDep(deps)); }, depHolder_);

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

    virtual const char* GetNodeType() const override
        { return "EventMerge"; }

    virtual int GetDependencyCount() const override
        { return sizeof...(TIns); }

private:
    template <typename U>
    void MergeFromDep(const std::shared_ptr<EventStreamNode<U>>& other)
    {
        this->Events().insert(this->Events().end(), other->Events().begin(), other->Events().end());
        other->DecrementPendingSuccessorCount();
    }

    std::tuple<std::shared_ptr<EventStreamNode<TIns>> ...> depHolder_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventSlotNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E>
class EventSlotNode : public EventStreamNode<E>
{
public:
    EventSlotNode(const std::shared_ptr<ReactiveGraph>& graphPtr, const std::shared_ptr<EventStreamNode<E>>& dep) :
        EventSlotNode::EventStreamNode( graphPtr ),
        slotInput_( *this, dep )
    {
        slotInput_.nodeId = GraphPtr()->RegisterNode(&slotInput_, NodeCategory::dyninput);
        this->RegisterMe();

        this->AttachToMe(slotInput_.nodeId);
        this->AttachToMe(dep->GetNodeId());
    }

    ~EventSlotNode()
    {
        this->DetachFromMe(slotInput_.dep->GetNodeId());
        this->DetachFromMe(slotInput_.nodeId);

        this->UnregisterMe();
        GraphPtr()->UnregisterNode(slotInput_.nodeId);
    }

    virtual const char* GetNodeType() const override
        { return "EventSlot"; }

    virtual int GetDependencyCount() const override
        { return 2; }

    virtual UpdateResult Update(TurnId turnId, int successorCount) override
    {
        this->Events().insert(this->Events().end(), slotInput_.dep->Events().begin(), slotInput_.dep->Events().end());

        slotInput_.dep->DecrementPendingSuccessorCount();

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

    void SetInput(const std::shared_ptr<EventStreamNode<E>>& newInput)
        { slotInput_.newDep = newInput; }

    NodeId GetInputNodeId() const
        { return slotInput_.nodeId; }

private:        
    struct VirtualInputNode : public IReactiveNode
    {
        VirtualInputNode(EventSlotNode& parentIn, const std::shared_ptr<EventStreamNode<E>>& depIn) :
            parent( parentIn ),
            dep( depIn )
            { }

        virtual const char* GetNodeType() const override
            { return "EventSlotVirtualInput"; }

        virtual int GetDependencyCount() const override
            { return 0; }

        virtual UpdateResult Update(TurnId turnId, int successorCount) override
        {
            if (dep != newDep)
            {
                parent.DynamicDetachFromMe(dep->GetNodeId(), 0);
                parent.DynamicAttachToMe(newDep->GetNodeId(), 0);

                dep = std::move(newDep);
                return UpdateResult::changed;
            }
            else
            {
                newDep.reset();
                return UpdateResult::unchanged;
            }
        }

        EventSlotNode& parent;

        NodeId nodeId;

        std::shared_ptr<EventStreamNode<E>> dep;
        std::shared_ptr<EventStreamNode<E>> newDep;
    };

    VirtualInputNode slotInput_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventProcessingNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TOut, typename TIn, typename F>
class EventProcessingNode : public EventStreamNode<TOut>
{
public:
    template <typename FIn>
    EventProcessingNode(const std::shared_ptr<ReactiveGraph>& graphPtr, FIn&& func, const std::shared_ptr<EventStreamNode<TIn>>& dep) :
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

    virtual UpdateResult Update(TurnId turnId, int successorCount) override
    {
        func_(EventRange<TIn>( dep_->Events() ), std::back_inserter(this->Events()));

        dep_->DecrementPendingSuccessorCount();

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
    SyncedEventProcessingNode(const std::shared_ptr<ReactiveGraph>& graphPtr, FIn&& func, const std::shared_ptr<EventStreamNode<TIn>>& dep, const std::shared_ptr<SignalNode<TSyncs>>& ... syncs) :
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

    virtual UpdateResult Update(TurnId turnId, int successorCount) override
    {
        // Updates might be triggered even if only sync nodes changed. Ignore those.
        if (dep_->Events().empty())
            return UpdateResult::unchanged;

        apply(
            [this] (const auto& ... syncs)
            {
                func_(EventRange<TIn>( this->dep_->Events() ), std::back_inserter(this->Events()), syncs->Value() ...);
            },
            syncHolder_);

        dep_->DecrementPendingSuccessorCount();

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

    virtual const char* GetNodeType() const override
        { return "SyncedEventProcessing"; }

    virtual int GetDependencyCount() const override
        { return 1 + sizeof...(TSyncs); }

private:
    F func_;

    std::shared_ptr<EventStreamNode<TIn>> dep_;

    std::tuple<std::shared_ptr<SignalNode<TSyncs>>...> syncHolder_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventJoinNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename ... Ts>
class EventJoinNode : public EventStreamNode<std::tuple<Ts ...>>
{
public:
    EventJoinNode(const std::shared_ptr<ReactiveGraph>& graphPtr, const std::shared_ptr<EventStreamNode<Ts>>& ... deps) :
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

    virtual UpdateResult Update(TurnId turnId, int successorCount) override
    {
        // Move events into buffers.
        apply([this, turnId] (Slot<Ts>& ... slots) { REACT_EXPAND_PACK(FetchBuffer(turnId, slots)); }, slots_);

        while (true)
        {
            bool isReady = true;

            // All slots ready?
            apply(
                [this, &isReady] (Slot<Ts>& ... slots)
								{
                    // Todo: combine return values instead
                    REACT_EXPAND_PACK(CheckSlot(slots, isReady));
                },
                slots_);

            if (!isReady)
                break;

            // Pop values from buffers and emit tuple.
            apply(
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
        slot.buffer.insert(slot.buffer.end(), slot.source->Events().begin(), slot.source->Events().end());
        slot.source->DecrementPendingSuccessorCount();
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
    EventLinkNode(const std::shared_ptr<ReactiveGraph>& graphPtr, const std::shared_ptr<ReactiveGraph>& srcGraphPtr, const std::shared_ptr<EventStreamNode<E>>& dep) :
        EventLinkNode::EventStreamNode( graphPtr ),
        linkOutput_( srcGraphPtr, dep )
    {
        this->RegisterMe(NodeCategory::input);
    }

    ~EventLinkNode()
    {
        this->UnregisterMe();
    }

		void SetWeakSelfPtr(const std::weak_ptr<EventLinkNode>& self)
				{	linkOutput_.parent = self; }

    virtual const char* GetNodeType() const override
        { return "EventLink"; }

    virtual int GetDependencyCount() const override
        { return 1; }

    virtual UpdateResult Update(TurnId turnId, int successorCount) override
		{
				this->SetPendingSuccessorCount(successorCount);
				return UpdateResult::changed;
		}

    void SetEvents(EventLinkNode::EventStreamNode::StorageType&& events)
        { this->Events() = std::move(events); }

private:
    struct VirtualOutputNode : public ILinkOutputNode
    {
        VirtualOutputNode(const std::shared_ptr<ReactiveGraph>& srcGraphPtrIn, const std::shared_ptr<EventStreamNode<E>>& depIn) :
						parent( ),
            srcGraphPtr( srcGraphPtrIn ),
            dep( depIn )
        {
					nodeId = srcGraphPtr->RegisterNode(this, NodeCategory::linkoutput);
					srcGraphPtr->OnNodeAttach(nodeId, dep->GetNodeId());
				}

				~VirtualOutputNode()
				{
						srcGraphPtr->OnNodeDetach(nodeId, dep->GetNodeId());
						srcGraphPtr->UnregisterNode(nodeId);
				}

        virtual const char* GetNodeType() const override
            { return "EventLinkOutput"; }

        virtual int GetDependencyCount() const override
            { return 1; }

        virtual UpdateResult Update(TurnId turnId, int successorCount) override
        {            
						return UpdateResult::changed;
        }

        virtual void CollectOutput(LinkOutputMap& output) override
        {
					if (auto p = parent.lock())
					{
						auto* rawPtr = p->GraphPtr().get();
						output[rawPtr].push_back(
							[storedParent = std::move(p), storedEvents = dep->Events()] () mutable
							{
								NodeId nodeId = storedParent->GetNodeId();
								auto& graphPtr = storedParent->GraphPtr();

								graphPtr->AddInput(nodeId,
									[&storedParent, &storedEvents]
									{
										storedParent->SetEvents(std::move(storedEvents));
									});
							});
					}
				}

				std::weak_ptr<EventLinkNode> parent;

        NodeId nodeId;

        std::shared_ptr<EventStreamNode<E>> dep;

        std::shared_ptr<ReactiveGraph> srcGraphPtr;
    };

    VirtualOutputNode linkOutput_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_GRAPH_EVENTNODES_H_INCLUDED