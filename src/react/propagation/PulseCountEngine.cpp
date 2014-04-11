
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "react/propagation/PulseCountEngine.h"

#include <cstdint>
#include <utility>

#include "react/common/Types.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/
namespace pulsecount {

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MarkerTask
///////////////////////////////////////////////////////////////////////////////////////////////////
class MarkerTask: public task
{
public:
    using StackT = NodeBuffer<Node,8>;

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
            Node& node = splitCount > 3 ? *nodes_.PopBack() : *nodes_.PopFront();

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
    StackT  nodes_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// UpdaterTask
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurn>
class UpdaterTask: public task
{
public:
    using BufferT = NodeBuffer<Node,8>;

    template <typename TInput>
    UpdaterTask(TTurn& turn, TInput srcBegin, TInput srcEnd) :
        turn_{ turn },
        nodes_{ srcBegin, srcEnd }
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
            Node& node = splitCount > 3 ? *nodes_.PopBack() : *nodes_.PopFront();

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
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// PulseCountEngine
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

//static vector<Node*> markNodes_;

template <typename TTurn>
void EngineBase<TTurn>::OnTurnPropagate(TTurn& turn)
{
    if (changedInputs_.size() <= 8)
    {
        auto& markerTask = *new(rootTask_->allocate_child())
            MarkerTask(changedInputs_.begin(), changedInputs_.end());

        rootTask_->set_ref_count(2);
        rootTask_->spawn_and_wait_for_all(markerTask);
    }
    else
    {
        REACT_ASSERT(false, "not implemented yet\n");
    }

    auto& updaterTask = *new(rootTask_->allocate_child())
        UpdaterTask<TTurn>(turn, changedInputs_.begin(), changedInputs_.end());

    rootTask_->set_ref_count(2);
    rootTask_->spawn_and_wait_for_all(updaterTask);

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
}// ~oldParent.ShiftMutex (write)

// Explicit instantiation
template class EngineBase<Turn>;
template class EngineBase<DefaultQueueableTurn<Turn>>;

} // ~namespace pulsecount
/****************************************/ REACT_IMPL_END /***************************************/