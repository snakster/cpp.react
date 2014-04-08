
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "react/propagation/PulseCountEngine.h"

#include <cstdint>
#include <utility>

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
}

template <typename TTurn>
void EngineBase<TTurn>::OnTurnPropagate(TTurn& turn)
{
    auto& mainTask = *new(rootTask_->allocate_child())
        NodeMarkerTask(changedInputs_.size(), changedInputs_.begin(), changedInputs_.end());

    rootTask_->set_ref_count(2);
    rootTask_->spawn_and_wait_for_all(mainTask);

    for (auto* node : changedInputs_)
        nudgeChildren(*node, true, turn);
    tasks_.wait();

    changedInputs_.clear();
}

template <typename TTurn>
void EngineBase<TTurn>::OnNodePulse(Node& node, TTurn& turn)
{
    nudgeChildren(node, true, turn);
}

template <typename TTurn>
void EngineBase<TTurn>::OnNodeIdlePulse(Node& node, TTurn& turn)
{
    nudgeChildren(node, false, turn);
}

template <typename TTurn>
void EngineBase<TTurn>::OnDynamicNodeAttach(Node& node, Node& parent, TTurn& turn)
{
    bool shouldTick = false;

    {// parent.ShiftMutex (write)
        NodeShiftMutexT::scoped_lock    lock(parent.ShiftMutex, true);
        
        parent.Successors.Add(node);

        // Has already been ticked & nudged its neighbors? (Note: Input nodes are always ready)
        if (! parent.Marked)
        {
            shouldTick = true;
        }
        else
        {
            node.SetCounter(1);
            node.ShouldUpdate = true;
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

class NodeMarkerTask: public task
{
public:
    template <typename TInput>
    NodeMarkerTask(size_t size, TInput srcBegin, TInput srcEnd) :
        Size{ size }
    {
        std::copy(srcBegin, srcEnd, nodes_.begin());
    }

    task* execute()
    {
        do
        {
            Node* node = nullptr;

            if (Size > 0)
            {
                node = nodes_[--Size];
            }
            else
            {
                break;
            }

            // Increment counter of each successor and add it to smaller stack
            for (auto* succ : node->Successors)
            {
                succ->IncCounter();

                // Skip if already marked
                if (succ->Marked.exchange(true))
                    continue;
                
                nodes_[Size++] = succ;

                if (Size == 8)
                {
                    //Delegate stack to new task
                    auto& t = *new(task::allocate_additional_child_of(*parent()))
                        NodeMarkerTask(4, nodes_.begin() + 4, nodes_.end());
                    spawn(t);

                    Size = 4;
                }
            }
        }
        while (true);

        return nullptr;
    }

    int Size = 0;
    std::array<Node*,8>     nodes_;

private:
    
};

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

template <typename TTurn>
void EngineBase<TTurn>::processChild(Node& node, bool update, TTurn& turn)
{
    // Invalidated, this node has to be ticked
    if (node.ShouldUpdate)
    {
        // Reset flag
        node.ShouldUpdate = false;
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
            succ->ShouldUpdate = true;

        // Delay tick?
        if (succ->DecCounter())
            continue;

        tasks_.run(std::bind(&EngineBase<TTurn>::processChild, this, std::ref(*succ), update, std::ref(turn)));
    }

    node.Marked = false;
}// ~node.ShiftMutex (read)

// Explicit instantiation
template class EngineBase<Turn>;
template class EngineBase<DefaultQueueableTurn<Turn>>;

} // ~namespace pulsecount
/****************************************/ REACT_IMPL_END /***************************************/