
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "react/propagation/ELMEngine.h"

#include <cstdint>

#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"

#include "react/common/Types.h"
#include "react/common/GraphData.h"

////////////////////////////////////////////////////////////////////////////////////////
REACT_IMPL_BEGIN
namespace elm {

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
	Counter(0),
	ShouldUpdate(false)
{
}

////////////////////////////////////////////////////////////////////////////////////////
/// EngineBase
////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurn>
void EngineBase<TTurn>::OnNodeCreate(Node& node)
{
	if (node.IsInputNode())
		inputNodes_.insert(&node);
}

template <typename TTurn>
void EngineBase<TTurn>::OnNodeDestroy(Node& node)
{
	// NOTE: Called from dtor, vtables have already been changed and IsInputNode will never return true
	// maybe change this
	inputNodes_.erase(&node);
}

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
	node.LastTurnId = turn.Id();
}

template <typename TTurn>
void EngineBase<TTurn>::OnTurnPropagate(TTurn& turn)
{
	for (auto* node : inputNodes_)
		nudgeChildren(*node, node->LastTurnId == turn.Id(), turn);

	tasks_.wait();
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

	{// oldParent.ShiftMutex
		NodeShiftMutexT::scoped_lock	lock(oldParent.ShiftMutex);

		oldParent.Successors.Remove(node);
	}// ~oldParent.ShiftMutex

	{// newParent.ShiftMutex
		NodeShiftMutexT::scoped_lock	lock(newParent.ShiftMutex);
		
		newParent.Successors.Add(node);

		if (newParent.LastTurnId == turn.Id())
		{
			shouldTick = true;
		}
		else
		{
			node.ShouldUpdate = true;
			node.Counter = node.DependencyCount() - 1;
		}
	}// ~newParent.ShiftMutex

	if (shouldTick)
		node.Tick(&turn);
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
{// node.ShiftMutex
	NodeShiftMutexT::scoped_lock	lock(node.ShiftMutex);

	for (auto* succ : node.Successors)
	{
		if (update)
			succ->ShouldUpdate = true;

		// Delay tick?
		if (++succ->Counter < succ->DependencyCount())
			continue;

		succ->Counter = 0;
		tasks_.run(std::bind(&EngineBase<TTurn>::processChild, this, std::ref(*succ), update, std::ref(turn)));
	}

	node.LastTurnId = turn.Id();	
}// ~node.ShiftMutex

// Explicit instantiation
template class EngineBase<Turn>;
template class EngineBase<DefaultQueueableTurn<Turn>>;

} // ~namespace elm
REACT_IMPL_END