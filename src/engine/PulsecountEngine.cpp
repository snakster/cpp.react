
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

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MarkerTask
///////////////////////////////////////////////////////////////////////////////////////////////////
class MarkerTask: public task
{
public:
    using BufferT = NodeBuffer<Node,chunk_size>;

    template <typename TInput>
    MarkerTask(TInput srcBegin, TInput srcEnd) :
        nodes_( srcBegin, srcEnd )
    {}

    MarkerTask(MarkerTask& other, SplitTag) :
        nodes_( other.nodes_, SplitTag( ) )
    {}

    task* execute()
    {
        uint splitCount = 0;

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
class UpdaterTask: public task
{
public:
    using BufferT = NodeBuffer<Node,chunk_size>;

    template <typename TInput>
    UpdaterTask(Turn& turn, TInput srcBegin, TInput srcEnd) :
        turn_( turn ),
        nodes_( srcBegin, srcEnd )
    {}

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

            if (node.Mark() == ENodeMark::should_update)
                node.Tick(&turn_);

            // Defer if node was dynamically attached to a predecessor that
            // has not pulsed yet
            if (node.State == ENodeState::dyn_defer)
                continue;

            // Repeat the update if a node was dynamically attached to a
            // predecessor that has already pulsed
            while (node.State == ENodeState::dyn_repeat)
                node.Tick(&turn_);

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
    Turn&   turn_;
    BufferT nodes_;
};


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
}

void EngineBase::OnNodeDetach(Node& node, Node& parent)
{
    parent.Successors.Remove(node);
}

void EngineBase::OnInputChange(Node& node, Turn& turn)
{
    changedInputs_.push_back(&node);
    node.State = ENodeState::changed;
}

template <typename TTask, typename TIt, typename ... TArgs>
void spawnTasks
(
    task& rootTask, task_list& spawnList,
    const size_t count, TIt start, TIt end,
    TArgs& ... args
)
{
    assert(1 + count <=
        static_cast<size_t>((std::numeric_limits<int>::max)()));

    rootTask.set_ref_count(1 + static_cast<int>(count));

    for (size_t i=0; i < (count - 1); i++)
    {
        spawnList.push_back(*new(rootTask.allocate_child())
            TTask(args ..., start, start + chunk_size));
        start += chunk_size;
    }

    spawnList.push_back(*new(rootTask.allocate_child())
        TTask(args ..., start, end));

    rootTask.spawn_and_wait_for_all(spawnList);

    spawnList.clear();
}

void EngineBase::Propagate(Turn& turn)
{
    const size_t initialTaskCount = (changedInputs_.size() - 1) / chunk_size + 1;

    spawnTasks<MarkerTask>(rootTask_, spawnList_, initialTaskCount,
        changedInputs_.begin(), changedInputs_.end());

    spawnTasks<UpdaterTask>(rootTask_, spawnList_, initialTaskCount,
        changedInputs_.begin(), changedInputs_.end(), turn);

    changedInputs_.clear();
}

void EngineBase::OnNodePulse(Node& node, Turn& turn)
{
    node.State = ENodeState::changed;
}

void EngineBase::OnNodeIdlePulse(Node& node, Turn& turn)
{
    node.State = ENodeState::unchanged;
}

void EngineBase::OnDynamicNodeAttach(Node& node, Node& parent, Turn& turn)
{// parent.ShiftMutex (write)
    NodeShiftMutexT::scoped_lock lock(parent.ShiftMutex, true);
        
    parent.Successors.Add(node);

    // Has already nudged its neighbors?
    if (parent.Mark() == ENodeMark::unmarked)
    {
        node.State = ENodeState::dyn_repeat;
    }
    else
    {
        node.State = ENodeState::dyn_defer;
        node.IncCounter();
        node.SetMark(ENodeMark::should_update);
    }
}// ~parent.ShiftMutex (write)

void EngineBase::OnDynamicNodeDetach(Node& node, Node& parent, Turn& turn)
{// parent.ShiftMutex (write)
    NodeShiftMutexT::scoped_lock lock(parent.ShiftMutex, true);

    parent.Successors.Remove(node);
}// ~parent.ShiftMutex (write)

} // ~namespace pulsecount
/****************************************/ REACT_IMPL_END /***************************************/