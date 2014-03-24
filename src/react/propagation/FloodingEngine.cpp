
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "react/propagation/FloodingEngine.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/
namespace flooding {

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Turn
///////////////////////////////////////////////////////////////////////////////////////////////////
Turn::Turn(TurnIdT id, TurnFlagsT flags) :
    TurnBase(id, flags)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Node
///////////////////////////////////////////////////////////////////////////////////////////////////
Node::Node() :
    isScheduled_(false),
    isProcessing_(false),
    shouldReprocess_(false)
{
}

bool Node::MarkForSchedule()
{
    if (IsOutputNode())
        return true;
    
    return isScheduled_.exchange(true, std::memory_order_relaxed) != true;
}

bool Node::Evaluate(Turn& turn)
{
    isScheduled_.store(false, std::memory_order_relaxed);

    {// mutex_
        EvalMutexT::scoped_lock    lock(mutex_);

        if (isProcessing_)
        {
            // Already processing
            // Tell task which is processing it to reprocess after it's done
            shouldReprocess_ = true;
            return false;
        }
        else
        {
            isProcessing_ = true;
        }
    }// ~mutex_

    Tick(&turn);

    bool result = false;

    {// mutex_
        EvalMutexT::scoped_lock    lock(mutex_);

        isProcessing_ = false;

        result = shouldReprocess_;
        shouldReprocess_ = false;
    }// ~mutex_

    return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EngineBase
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
    // Non-input nodes
    for (auto* node : changedInputs_)
        pulse(*node, turn);
    tasks_.wait();

    // Input nodes
    for (auto node : outputNodes_)
        tasks_.run(std::bind(&Node::Tick, node, &turn));

    tasks_.wait();

    outputNodes_.clear();
    changedInputs_.clear();
}

template <typename TTurn>
void EngineBase<TTurn>::OnNodePulse(Node& node, TTurn& turn)
{
    pulse(node, turn);
}

template <typename TTurn>
void EngineBase<TTurn>::OnDynamicNodeAttach(Node& node, Node& parent, TTurn& turn)
{
    {// parent.ShiftMutex
        NodeShiftMutexT::scoped_lock    lock(parent.ShiftMutex);
    
        parent.Successors.Add(node);
    }// ~parent.ShiftMutex

    // Called from Tick, so we already have exclusive access to the node.
    // Just tick again to recalc the value.
    node.Tick(&turn);
}

template <typename TTurn>
void EngineBase<TTurn>::OnDynamicNodeDetach(Node& node, Node& parent, TTurn& turn)
{// parent.ShiftMutex
    NodeShiftMutexT::scoped_lock    lock(parent.ShiftMutex);

    parent.Successors.Remove(node);
}// ~parent.ShiftMutex

template <typename TTurn>
void EngineBase<TTurn>::pulse(Node& node, TTurn& turn)
{// node.ShiftMutex
    NodeShiftMutexT::scoped_lock    lock(node.ShiftMutex);

    for (auto* succ : node.Successors)
        if (succ->MarkForSchedule())
            tasks_.run(std::bind(&EngineBase<TTurn>::process, this, std::ref(*succ), std::ref(turn)));
}// ~node.ShiftMutex

template <typename TTurn>
void EngineBase<TTurn>::process(Node& node, TTurn& turn)
{
    if (! node.IsOutputNode())
    {
        bool shouldRepeat = false;
        do
            shouldRepeat = node.Evaluate(turn);
        while (shouldRepeat);
    }
    else
    {// outputMutex_
        OutputMutexT::scoped_lock    lock(outputMutex_);

        outputNodes_.insert(&node);        
    }// ~outputMutex_
}

// Explicit instantiation
template class EngineBase<Turn>;
template class EngineBase<DefaultQueueableTurn<Turn>>;

} // ~namespace flooding
/****************************************/ REACT_IMPL_END /***************************************/