
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
static const uint heavy_weight  = 1000;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Turn
///////////////////////////////////////////////////////////////////////////////////////////////////
Turn::Turn(TurnIdT id, TurnFlagsT flags) :
    TurnBase(id, flags)
{}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// PulsecountEngine
///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename TTurn>
void EngineBase<TTurn>::OnNodeAttach(Node& node, Node& parent)
{
    parent.Successors.Add(node);

    if (node.Level <= parent.Level)
        node.Level = parent.Level + 1;
}

template <typename TTurn>
void EngineBase<TTurn>::OnNodeDetach(Node& node, Node& parent)
{
    parent.Successors.Remove(node);
}

template <typename TTurn>
void EngineBase<TTurn>::OnTurnInputChange(Node& node, TTurn& turn)
{
    processChildren(node, turn);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// UpdaterTask
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurn>
class UpdaterTask: public task
{
public:
    using BufferT = NodeBuffer<Node,chunk_size>;

    UpdaterTask(TTurn& turn, Node* node) :
        turn_{ turn },
        nodes_{ node }
    {}

    UpdaterTask(UpdaterTask& other, SplitTag) :
        turn_{ other.turn_ },
        nodes_{ other.nodes_, SplitTag{} }
    {}

    task* execute()
    {
        int splitCount = 0;

        while (!nodes_.IsEmpty())
        {
            Node& node = splitCount > dfs_threshold ? *nodes_.PopBack() : *nodes_.PopFront();

            if (node.IsInitial() || node.ShouldUpdate())
                node.Tick(&turn_);

            node.ClearInitialFlag();
            node.SetShouldUpdate(false);

            // Skip deferred node
            if (node.IsDeferred())
            {
                node.ClearDeferredFlag();
                continue;
            }

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

                    succ->SetReadyCount(0);

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
                            splitCount++;

                            //Delegate half the work to new task
                            auto& t = *new(task::allocate_additional_child_of(*parent()))
                                UpdaterTask(*this, SplitTag{});

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
    TTurn&  turn_;
    BufferT nodes_;
};

template <typename TTurn>
void EngineBase<TTurn>::OnTurnPropagate(TTurn& turn)
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

    rootTask_->set_ref_count(1 + subtreeRoots_.size());

    for (auto* node : subtreeRoots_)
    {
        // Ignore if root flag has been cleared because node was part of another subtree
        if (! node->IsRoot())
        {
            rootTask_->decrement_ref_count();
            continue;
        }

        spawnList_.push_back(*new(rootTask_->allocate_child())
            UpdaterTask<TTurn>(turn, node));

        node->ClearRootFlag();
    }

    rootTask_->spawn_and_wait_for_all(spawnList_);

    subtreeRoots_.clear();
    spawnList_.clear();

    isInPhase2_ = false;
}

template <typename TTurn>
void EngineBase<TTurn>::OnNodePulse(Node& node, TTurn& turn)
{
    if (isInPhase2_)
        node.SetChangedFlag();
    else
        processChildren(node, turn);
}

template <typename TTurn>
void EngineBase<TTurn>::OnNodeIdlePulse(Node& node, TTurn& turn)
{
    if (isInPhase2_)
        node.ClearChangedFlag();
}

template <typename TTurn>
void EngineBase<TTurn>::OnDynamicNodeAttach(Node& node, Node& parent, TTurn& turn)
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

template <typename TTurn>
void EngineBase<TTurn>::OnDynamicNodeDetach(Node& node, Node& parent, TTurn& turn)
{
    if (isInPhase2_)
        applyAsyncDynamicDetach(node, parent, turn);
    else
        OnNodeDetach(node, parent);
}

template <typename TTurn>
void EngineBase<TTurn>::applyAsyncDynamicAttach(Node& node, Node& parent, TTurn& turn)
{
    bool shouldTick = false;

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
            shouldTick = true;
        }
        else
        {
            node.SetDeferredFlag();
            node.SetShouldUpdate(true);

            node.WaitCount = 1;
        }
    }// ~parent.ShiftMutex (write)

    if (shouldTick)
        node.Tick(&turn);
}

template <typename TTurn>
void EngineBase<TTurn>::applyAsyncDynamicDetach(Node& node, Node& parent, TTurn& turn)
{// parent.ShiftMutex (write)
    NodeShiftMutexT::scoped_lock    lock(parent.ShiftMutex, true);

    parent.Successors.Remove(node);
}// ~parent.ShiftMutex (write)

template <typename TTurn>
void EngineBase<TTurn>::processChildren(Node& node, TTurn& turn)
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

template <typename TTurn>
void EngineBase<TTurn>::markSubtree(Node& root)
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

template <typename TTurn>
void EngineBase<TTurn>::invalidateSuccessors(Node& node)
{
    for (auto* succ : node.Successors)
    {
        if (succ->NewLevel <= node.Level)
            succ->NewLevel = node.Level + 1;
    }
}

// Explicit instantiation
template class EngineBase<Turn>;
template class EngineBase<DefaultQueueableTurn<Turn>>;

} // ~namespace subtree
/****************************************/ REACT_IMPL_END /***************************************/