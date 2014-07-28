
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "react/engine/SubtreeEngine.h"

#include <cstdint>
#include <utility>

#include "react/common/Types.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/
namespace subtree {

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Parameters
///////////////////////////////////////////////////////////////////////////////////////////////////
static const uint chunk_size    = 8;
static const uint dfs_threshold = 3;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Turn
///////////////////////////////////////////////////////////////////////////////////////////////////
Turn::Turn(TurnIdT id, TransactionFlagsT flags) :
    TurnBase( id, flags )
{}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// PulsecountEngine
///////////////////////////////////////////////////////////////////////////////////////////////////

void EngineBase::OnNodeAttach(Node& node, Node& parent)
{
    parent.Successors.Add(node);

    if (node.Level <= parent.Level)
        node.Level = parent.Level + 1;
}

void EngineBase::OnNodeDetach(Node& node, Node& parent)
{
    parent.Successors.Remove(node);
}

void EngineBase::OnInputChange(Node& node, Turn& turn)
{
    processChildren(node, turn);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// UpdaterTask
///////////////////////////////////////////////////////////////////////////////////////////////////
class UpdaterTask: public task
{
public:
    using BufferT = NodeBuffer<Node,chunk_size>;

    UpdaterTask(Turn& turn, Node* node) :
        turn_( turn ),
        nodes_( node )
    {}

    UpdaterTask(UpdaterTask& other, SplitTag) :
        turn_( other.turn_ ),
        nodes_( other.nodes_, SplitTag( ) )
    {}

    task* execute()
    {
        uint splitCount = 0;

        while (!nodes_.IsEmpty())
        {
            Node& node = splitCount > dfs_threshold ? *nodes_.PopBack() : *nodes_.PopFront();

            if (node.IsInitial() || node.ShouldUpdate())
                node.Tick(&turn_);

            node.ClearInitialFlag();
            node.SetShouldUpdate(false);

            // Defer if node was dynamically attached to a predecessor that
            // has not pulsed yet
            if (node.IsDeferred())
            {
                node.ClearDeferredFlag();
                continue;
            }

            // Repeat the update if a node was dynamically attached to a
            // predecessor that has already pulsed
            while (node.IsRepeated())
            {
                node.ClearRepeatedFlag();
                node.Tick(&turn_);
            }

            node.SetReadyCount(0);

            // Mark successors for update?
            bool update = node.IsChanged();

            {// node.ShiftMutex
                Node::ShiftMutexT::scoped_lock lock(node.ShiftMutex, false);

                for (auto* succ : node.Successors)
                {
                    if (update)
                        succ->SetShouldUpdate(true);

                    // Wait for more?
                    if (succ->IncReadyCount())
                        continue;

                    // Heavyweight - spawn new task
                    if (succ->IsHeavyweight())
                    {
                        auto& t = *new(task::allocate_additional_child_of(*parent()))
                            UpdaterTask(turn_, succ);

                        spawn(t);
                    }
                    // Leightweight - add to buffer, split if full
                    else
                    {
                        nodes_.PushBack(succ);

                        if (nodes_.IsFull())
                        {
                            ++splitCount;

                            //Delegate half the work to new task
                            auto& t = *new(task::allocate_additional_child_of(*parent()))
                                UpdaterTask(*this, SplitTag());

                            spawn(t);
                        }
                    }
                }

                node.ClearMarkedFlag();
            }// ~node.ShiftMutex
        }

        return nullptr;
    }

private:
    Turn&   turn_;
    BufferT nodes_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EngineBase
///////////////////////////////////////////////////////////////////////////////////////////////////
void EngineBase::Propagate(Turn& turn)
{
    // Phase 1
    while (scheduledNodes_.FetchNext())
    {
        for (auto* curNode : scheduledNodes_.NextValues())
        {
            if (curNode->Level < curNode->NewLevel)
            {
                curNode->Level = curNode->NewLevel;
                invalidateSuccessors(*curNode);
                scheduledNodes_.Push(curNode);
                continue;
            }

            curNode->ClearQueuedFlag();
            curNode->Tick(&turn);
        }
    }

    // Phase 2
    isInPhase2_ = true;

    assert((1 + subtreeRoots_.size()) <=
        static_cast<size_t>((std::numeric_limits<int>::max)()));

    rootTask_.set_ref_count(1 + static_cast<int>(subtreeRoots_.size()));

    for (auto* node : subtreeRoots_)
    {
        // Ignore if root flag has been cleared because node was part of another subtree
        if (! node->IsRoot())
        {
            rootTask_.decrement_ref_count();
            continue;
        }

        spawnList_.push_back(*new(rootTask_.allocate_child())
            UpdaterTask(turn, node));

        node->ClearRootFlag();
    }

    rootTask_.spawn_and_wait_for_all(spawnList_);

    subtreeRoots_.clear();
    spawnList_.clear();

    isInPhase2_ = false;
}

void EngineBase::OnNodePulse(Node& node, Turn& turn)
{
    if (isInPhase2_)
        node.SetChangedFlag();
    else
        processChildren(node, turn);
}

void EngineBase::OnNodeIdlePulse(Node& node, Turn& turn)
{
    if (isInPhase2_)
        node.ClearChangedFlag();
}

void EngineBase::OnDynamicNodeAttach(Node& node, Node& parent, Turn& turn)
{
    if (isInPhase2_)
    {
        applyAsyncDynamicAttach(node, parent, turn);
    }
    else
    {
        OnNodeAttach(node, parent);
    
        invalidateSuccessors(node);

        // Re-schedule this node
        node.SetQueuedFlag();
        scheduledNodes_.Push(&node);
    }
}

void EngineBase::OnDynamicNodeDetach(Node& node, Node& parent, Turn& turn)
{
    if (isInPhase2_)
        applyAsyncDynamicDetach(node, parent, turn);
    else
        OnNodeDetach(node, parent);
}

void EngineBase::applyAsyncDynamicAttach(Node& node, Node& parent, Turn& turn)
{// parent.ShiftMutex (write)
    NodeShiftMutexT::scoped_lock    lock(parent.ShiftMutex, true);
        
    parent.Successors.Add(node);

    // Level recalulation applied when added to topoqueue next time.
    // During the async phase 2 it's not needed.
    if (node.NewLevel <= parent.Level)
        node.NewLevel = parent.Level + 1;

    // Has already nudged its neighbors?
    if (! parent.IsMarked())
    {
        node.SetRepeatedFlag();
    }
    else
    {
        node.SetDeferredFlag();
        node.SetShouldUpdate(true);
        node.DecReadyCount();
    }
}// ~parent.ShiftMutex (write)

void EngineBase::applyAsyncDynamicDetach(Node& node, Node& parent, Turn& turn)
{// parent.ShiftMutex (write)
    NodeShiftMutexT::scoped_lock    lock(parent.ShiftMutex, true);

    parent.Successors.Remove(node);
}// ~parent.ShiftMutex (write)

void EngineBase::processChildren(Node& node, Turn& turn)
{
    // Add children to queue
    for (auto* succ : node.Successors)
    {
        // Ignore if node part of marked subtree
        if (succ->IsMarked())
            continue;

        // Light nodes use sequential toposort in phase 1
        if (! succ->IsHeavyweight())
        {
            if (!succ->IsQueued())
            {
                succ->SetQueuedFlag();
                scheduledNodes_.Push(succ);
            }
        }
        // Heavy nodes + subtrees are deferred for parallel updating in phase 2
        else
        {
            // Force an initial update for heavy non-input nodes.
            // (non-atomic flag, unlike ShouldUpdate)
            if (!succ->IsInputNode())
                succ->SetInitialFlag();

            succ->SetChangedFlag();
            succ->SetRootFlag();

            markSubtree(*succ);

            subtreeRoots_.push_back(succ);
        }
    }
}

void EngineBase::markSubtree(Node& root)
{
    root.SetMarkedFlag();
    root.WaitCount = 0;

    for (auto* succ : root.Successors)
    {
        if (!succ->IsMarked())
            markSubtree(*succ);

        // Successor of another marked node? -> not a root anymore
        else if (succ->IsRoot())
            succ->ClearRootFlag();

        ++succ->WaitCount;
    }
}

void EngineBase::invalidateSuccessors(Node& node)
{
    for (auto* succ : node.Successors)
    {
        if (succ->NewLevel <= node.Level)
            succ->NewLevel = node.Level + 1;
    }
}

} // ~namespace subtree
/****************************************/ REACT_IMPL_END /***************************************/