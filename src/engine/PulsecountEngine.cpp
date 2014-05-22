
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "react/engine/PulsecountEngine.h"

#include <cstdint>
#include <utility>

#include "react/common/Types.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/
namespace pulsecount {

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Constants
///////////////////////////////////////////////////////////////////////////////////////////////////
static const uint chunk_size    = 8;
static const uint dfs_threshold = 3;
static const uint heavy_weight  = 1000;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MarkerTask
///////////////////////////////////////////////////////////////////////////////////////////////////
class MarkerTask: public task
{
public:
    using BufferT = NodeBuffer<Node,chunk_size>;

    template <typename TInput>
    MarkerTask(TInput srcBegin, TInput srcEnd) :
        nodes_{ srcBegin, srcEnd }
    {}

    MarkerTask(MarkerTask& other, SplitTag) :
        nodes_{ other.nodes_, SplitTag{} }
    {}

    task* execute()
    {
        int splitCount = 0;

        while (! nodes_.IsEmpty())
        {
            Node& node = splitCount > dfs_threshold ? *nodes_.PopBack() : *nodes_.PopFront();

            // Increment counter of each successor and add it to smaller stack
            for (auto* succ : node.Successors)
            {
                succ->IncCounter();

                // Skip if already marked as reachable
                if (! succ->ExchangeMark(ENodeMark::visited))
                    continue;
                
                nodes_.PushBack(succ);

                if (nodes_.IsFull())
                {
                    splitCount++;

                    //Delegate half the work to new task
                    auto& t = *new(task::allocate_additional_child_of(*parent()))
                        MarkerTask(*this, SplitTag{});

                    spawn(t);
                }
            }
        }

        return nullptr;
    }

private:
    BufferT  nodes_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// UpdaterTask
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurn>
class UpdaterTask: public task
{
public:
    using BufferT = NodeBuffer<Node,chunk_size>;

    template <typename TInput>
    UpdaterTask(TTurn& turn, TInput srcBegin, TInput srcEnd) :
        turn_{ turn },
        nodes_{ srcBegin, srcEnd }
    {}

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

            if (node.Mark() == ENodeMark::should_update)
                node.Tick(&turn_);

            if (node.State == ENodeState::deferred)
                continue;

            // Mark successors for update?
            bool update = node.State == ENodeState::changed;
            node.State = ENodeState::unchanged;

            {// node.ShiftMutex
                Node::ShiftMutexT::scoped_lock lock(node.ShiftMutex, false);

                for (auto* succ : node.Successors)
                {
                    if (update)
                        succ->SetMark(ENodeMark::should_update);

                    // Delay tick?
                    if (succ->DecCounter())
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
                            splitCount++;

                            //Delegate half the work to new task
                            auto& t = *new(task::allocate_additional_child_of(*parent()))
                                UpdaterTask(*this, SplitTag{});

                            spawn(t);
                        }
                    }
                }

                node.SetMark(ENodeMark::unmarked);
            }// ~node.ShiftMutex
        }

        return nullptr;
    }

private:
    TTurn&  turn_;
    BufferT nodes_;
};


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
}

template <typename TTurn>
void EngineBase<TTurn>::OnNodeDetach(Node& node, Node& parent)
{
    parent.Successors.Remove(node);
}

template <typename TTurn>
void EngineBase<TTurn>::OnTurnInputChange(Node& node, TTurn& turn)
{
    changedInputs_.push_back(&node);
    node.State = ENodeState::changed;
}

template <typename TTask, typename TIt, typename ... TArgs>
void spawnHelper
(
    task* rootTask, task_list& spawnList,
    const int count, TIt start, TIt end,
    TArgs& ... args
)
{
    rootTask->set_ref_count(1 + count);

    for (int i=0; i < (count - 1); i++)
    {
        spawnList.push_back(*new(rootTask->allocate_child())
            TTask(args ..., start, start + chunk_size));
        start += chunk_size;
    }

    spawnList.push_back(*new(rootTask->allocate_child())
        TTask(args ..., start, end));

    rootTask->spawn_and_wait_for_all(spawnList);

    spawnList.clear();
}

template <typename TTurn>
void EngineBase<TTurn>::OnTurnPropagate(TTurn& turn)
{
    const int initialTaskCount = (changedInputs_.size() - 1) / chunk_size + 1;

    spawnHelper<MarkerTask>(rootTask_, spawnList_, initialTaskCount,
        changedInputs_.begin(), changedInputs_.end());

    spawnHelper<UpdaterTask<TTurn>>(rootTask_, spawnList_, initialTaskCount,
        changedInputs_.begin(), changedInputs_.end(), turn);

    changedInputs_.clear();
}

template <typename TTurn>
void EngineBase<TTurn>::OnNodePulse(Node& node, TTurn& turn)
{
    node.State = ENodeState::changed;
}

template <typename TTurn>
void EngineBase<TTurn>::OnNodeIdlePulse(Node& node, TTurn& turn)
{
    node.State = ENodeState::unchanged;
}

template <typename TTurn>
void EngineBase<TTurn>::OnDynamicNodeAttach(Node& node, Node& parent, TTurn& turn)
{
    bool shouldTick = false;

    {// parent.ShiftMutex (write)
        NodeShiftMutexT::scoped_lock    lock(parent.ShiftMutex, true);
        
        parent.Successors.Add(node);

        // Has already nudged its neighbors?
        if (parent.Mark() == ENodeMark::unmarked)
        {
            shouldTick = true;
        }
        else
        {
            node.State = ENodeState::deferred;
            node.SetCounter(1);
            node.SetMark(ENodeMark::should_update);
        }
    }// ~parent.ShiftMutex (write)

    if (shouldTick)
        node.Tick(&turn);
}

template <typename TTurn>
void EngineBase<TTurn>::OnDynamicNodeDetach(Node& node, Node& parent, TTurn& turn)
{// parent.ShiftMutex (write)
    NodeShiftMutexT::scoped_lock    lock(parent.ShiftMutex, true);

    parent.Successors.Remove(node);
}// ~parent.ShiftMutex (write)

// Explicit instantiation
template class EngineBase<Turn>;
template class EngineBase<DefaultQueueableTurn<Turn>>;

} // ~namespace pulsecount
/****************************************/ REACT_IMPL_END /***************************************/