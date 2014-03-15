
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "react/propagation/PulseCountEngine.h"

#include <cstdint>

#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"
#include "tbb/tick_count.h"

#include "react/common/Types.h"

/*********************************/ REACT_IMPL_BEGIN /*********************************/
namespace pulsecount {

////////////////////////////////////////////////////////////////////////////////////////
/// Turn
////////////////////////////////////////////////////////////////////////////////////////
Turn::Turn(TurnIdT id, TurnFlagsT flags) :
	TurnBase(id, flags)
{
}

////////////////////////////////////////////////////////////////////////////////////////
/// Node
////////////////////////////////////////////////////////////////////////////////////////
Node::Node() :
	Counter{ 0 },
	ShouldUpdate{ false },
	Marked{ false }
{
}

////////////////////////////////////////////////////////////////////////////////////////
/// PulseCountEngine
////////////////////////////////////////////////////////////////////////////////////////
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
	// Note: Copies the vector
	runInitReachableNodesTask(changedInputs_);
	tasks_.wait();

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
void EngineBase<TTurn>::OnNodeShift(Node& node, Node& oldParent, Node& newParent, TTurn& turn)
{
	bool shouldTick = false;
	
	{// oldParent.ShiftMutex (write)
		NodeShiftMutexT::scoped_lock	lock(oldParent.ShiftMutex, true);

		oldParent.Successors.Remove(node);
	}// ~oldParent.ShiftMutex (write)

	{// newParent.ShiftMutex (write)
		NodeShiftMutexT::scoped_lock	lock(newParent.ShiftMutex, true);
		
		newParent.Successors.Add(node);

		// Has already been ticked & nudged its neighbors? (Note: Input nodes are always ready)
		if (! newParent.Marked)
		{
			shouldTick = true;
		}
		else
		{
			node.Counter = 1;
			node.ShouldUpdate = true;
		}
	}// ~newParent.ShiftMutex (write)

	if (shouldTick)
		node.Tick(&turn);
}

template <typename TTurn>
void EngineBase<TTurn>::runInitReachableNodesTask(NodeVectorT leftNodes)
{
	NodeVectorT rightNodes;

	// Manage two balanced stacks of nodes.
	// Left size is always >= right size.
	// If left size exceeds threshold, move work to another task
	do
	{
		Node* node = nullptr;

		// Take a node from the larger stack, or exit if no more nodes
		if (leftNodes.size() > rightNodes.size())
		{
			node = leftNodes.back();
			leftNodes.pop_back();
		}
		else if (rightNodes.size() > 0)
		{
			node = rightNodes.back();
			rightNodes.pop_back();
		}
		else
		{
			break;
		}

		// Increment counter of each successor and add it to smaller stack
		for (auto* succ : node->Successors)
		{
			succ->Counter++;

			// Skip if already marked
			if (succ->Marked.exchange(true))
				continue;
				
			(leftNodes.size() > rightNodes.size() ? rightNodes : leftNodes).push_back(succ);

			if (leftNodes.size() > 4)
			{
				//Delegate stack to new task
				tasks_.run(std::bind(&EngineBase<TTurn>::runInitReachableNodesTask, this, std::move(leftNodes)));
				leftNodes.clear();
			}
		}
	}
	while (true);
}

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
	NodeShiftMutexT::scoped_lock	lock(node.ShiftMutex);

	// Select first child as next node, dispatch tasks for rest
	for (auto* succ : node.Successors)
	{
		if (update)
			succ->ShouldUpdate = true;

		// Delay tick?
		if (succ->Counter-- > 1)
			continue;

		tasks_.run(std::bind(&EngineBase<TTurn>::processChild, this, std::ref(*succ), update, std::ref(turn)));
	}

	node.Marked = false;
}// ~node.ShiftMutex (read)

// Explicit instantiation
template class EngineBase<Turn>;
template class EngineBase<DefaultQueueableTurn<Turn>>;

} // ~namespace pulsecount
/**********************************/ REACT_IMPL_END /**********************************/