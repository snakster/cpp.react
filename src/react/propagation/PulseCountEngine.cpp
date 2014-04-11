
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "react/propagation/PulseCountEngine.h"

#include <cstdint>
#include <utility>
#include <stack>

#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"
#include "tbb/tick_count.h"
#include "tbb/task.h"

using tbb::task;
using tbb::empty_task;


#include "react/common/Types.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/
namespace pulsecount {

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

static vector<Node*> markNodes_;

template <typename TTurn>
void EngineBase<TTurn>::OnTurnPropagate(TTurn& turn)
{
    //auto& markerTask = *new(rootTask_->allocate_child())
    //    MarkerTask(changedInputs_.begin(), changedInputs_.end());

    //rootTask_->set_ref_count(2);
    //rootTask_->spawn_and_wait_for_all(markerTask);

    for (auto* inputNode : changedInputs_)
    {
        // Increment counter of each successor and add it to smaller stack
        for (auto* succ : inputNode->Successors)
        {
            ++succ->ReachMax;

            // Skip if already marked as reachable
            if (succ->Visited)
                continue;

            succ->Visited = true;
            markNodes_.push_back(succ);
        }
    }

    while (! markNodes_.empty())
    {
        Node& node = *markNodes_.back();
        markNodes_.pop_back();

        // Increment counter of each successor and add it to smaller stack
        for (auto* succ : node.Successors)
        {
            ++succ->ReachMax;

            // Skip if already marked as reachable
            if (succ->Visited)
                continue;

            succ->Visited = true;
            markNodes_.push_back(succ);
        }
    }


    auto& updaterTask = *new(rootTask_->allocate_child())
        UpdaterTask<TTurn>(turn, changedInputs_.begin(), changedInputs_.end());

    rootTask_->set_ref_count(2);
    rootTask_->spawn_and_wait_for_all(updaterTask);

    //for (auto* node : changedInputs_)
//        nudgeChildren(*node, true, turn);
//    tasks_.wait();

    changedInputs_.clear();
}

template <typename TTurn>
void EngineBase<TTurn>::OnNodePulse(Node& node, TTurn& turn)
{
    node.State = ENodeState::changed;
//    nudgeChildren(node, true, turn);
}

template <typename TTurn>
void EngineBase<TTurn>::OnNodeIdlePulse(Node& node, TTurn& turn)
{
    node.State = ENodeState::unchanged;
//    nudgeChildren(node, false, turn);
}

template <typename TTurn>
void EngineBase<TTurn>::OnDynamicNodeAttach(Node& node, Node& parent, TTurn& turn)
{
    bool shouldTick = false;

    {// parent.ShiftMutex (write)
        NodeShiftMutexT::scoped_lock    lock(parent.ShiftMutex, true);
        
        parent.Successors.Add(node);

        // Has already been ticked & nudged its neighbors? (Note: Input nodes are always ready)
        if (parent.Mark() != ENodeMark::unmarked)
        {
            shouldTick = true;
        }
        else
        {
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


using NodeVectorT = vector<Node*>;



//class MarkerTask: public task
//{
//public:
//    using StackT = NodeBuffer<Node,8>;
//
//    template <typename TInput>
//    MarkerTask(TInput srcBegin, TInput srcEnd) :
//        nodes_{ srcBegin, srcEnd }
//    {}
//
//    MarkerTask(MarkerTask& other, SplitTag) :
//        nodes_{ other.nodes_, SplitTag{} }
//    {}
//
//    task* execute()
//    {
//        int splitCount = 0;
//
//        while (! nodes_.IsEmpty())
//        {
//            Node& node = splitCount > 3 ? *nodes_.PopBack() : *nodes_.PopFront();
//
//            // Increment counter of each successor and add it to smaller stack
//            for (auto* succ : node.Successors)
//            {
//                succ->IncCounter();
//
//                // Skip if already marked as reachable
//                if (! succ->ExchangeMark(ENodeMark::visited))
//                    continue;
//                
//                nodes_.PushBack(succ);
//
//                if (nodes_.IsFull())
//                {
//                    splitCount++;
//
//                    //Delegate half the work to new task
//                    auto& t = *new(task::allocate_additional_child_of(*parent()))
//                        MarkerTask(*this, SplitTag{});
//
//                    spawn(t);
//                }
//            }
//        }
//
//        return nullptr;
//    }
//
//private:
//    StackT  nodes_;
//};

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
                    //if (succ->DecCounter())
                        //continue;
                    int t = succ->ReachMax - 1;
                    if (succ->ReachCount++ < t)
                        continue;

                    succ->ReachMax = 0;
                    succ->ReachCount = 0;

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
                node.Visited = false;
            }// ~node.ShiftMutex
        }

        return nullptr;
    }

private:
    TTurn&  turn_;
    BufferT nodes_;
};



template <typename TTurn>
void EngineBase<TTurn>::processChild(Node& node, bool update, TTurn& turn)
{
    // Invalidated, this node has to be ticked
    if (node.Mark() == ENodeMark::should_update)
    {
        // Reset flag
        node.Tick(&turn);
    }
    // No tick required
    else
    {
        nudgeChildren(node, false, turn);
    }
}

template <typename TTurn>
void EngineBase<TTurn>::nudgeChildren(Node& node, bool update, TTurn& turn)
{// node.ShiftMutex (read)
    NodeShiftMutexT::scoped_lock    lock(node.ShiftMutex, false);

    // Select first child as next node, dispatch tasks for rest
    for (auto* succ : node.Successors)
    {
        if (update)
            succ->SetMark(ENodeMark::should_update);

        // Delay tick?
        if (succ->DecCounter())
            continue;

        tasks_.run(std::bind(&EngineBase<TTurn>::processChild, this, std::ref(*succ), update, std::ref(turn)));
    }

    node.SetMark(ENodeMark::unmarked);
}// ~node.ShiftMutex (read)

// Explicit instantiation
template class EngineBase<Turn>;
template class EngineBase<DefaultQueueableTurn<Turn>>;


//class NodeMarkerTask: public task
//{
//public:
//    NodeMarkerTask(NodeVectorT&& leftNodes) :
//        leftNodes_{ std::move(leftNodes) }
//    {}
//
//    task* execute()
//    {
//        do
//        {
//            Node* node = nullptr;
//
//            // Take a node from the larger stack, or exit if no more nodes
//            if (leftNodes_.size() > rightNodes_.size())
//            {
//                node = leftNodes_.back();
//                leftNodes_.pop_back();
//            }
//            else if (rightNodes_.size() > 0)
//            {
//                node = rightNodes_.back();
//                rightNodes_.pop_back();
//            }
//            else
//            {
//                break;
//            }
//
//            // Increment counter of each successor and add it to smaller stack
//            for (auto* succ : node->Successors)
//            {
//                succ->IncCounter();
//
//                // Skip if already marked
//                if (succ->Marked.exchange(true))
//                    continue;
//                
//                (leftNodes_.size() > rightNodes_.size() ?rightNodes_ : leftNodes_).push_back(succ);
//
//                if (leftNodes_.size() > 4)
//                {
//                    //Delegate stack to new task
//                    auto& t = *new(task::allocate_additional_child_of(*parent()))
//                        NodeMarkerTask(std::move(leftNodes_));
//                    spawn(t);
//                    leftNodes_.clear();
//                }
//            }
//        }
//        while (true);
//
//        return nullptr;
//    }
//
//private:
//    NodeVectorT     leftNodes_;
//    NodeVectorT     rightNodes_;
//};

//class NodeMarkerTask: public task
//{
//public:
//    NodeMarkerTask(NodeVectorT&& nodes) :
//        nodes_{ std::move(nodes) }
//    {}
//
//    task* execute()
//    {
//        do
//        {
//            Node* node = nullptr;
//
//            // Take a node from the larger stack, or exit if no more nodes
//            if (nodes_.size() > 0)
//            {
//                node = nodes_.back();
//                nodes_.pop_back();
//            }
//            else
//            {
//                break;
//            }
//
//            // Increment counter of each successor and add it to smaller stack
//            for (auto* succ : node->Successors)
//            {
//                succ->IncCounter();
//
//                // Skip if already marked
//                if (succ->Marked.exchange(true))
//                    continue;
//                
//                nodes_.push_back(succ);
//
//                if (nodes_.size() > 8)
//                {
//                    //Delegate stack to new task
//                    auto& t = *new(task::allocate_additional_child_of(*parent()))
//                        NodeMarkerTask(NodeVectorT{nodes_.begin()+4, nodes_.end()});
//                    spawn(t);
//                    nodes_.resize(4);
//                }
//            }
//        }
//        while (true);
//
//        return nullptr;
//    }
//
//private:
//    NodeVectorT     nodes_;
//};

} // ~namespace pulsecount
/****************************************/ REACT_IMPL_END /***************************************/