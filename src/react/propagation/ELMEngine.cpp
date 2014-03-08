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
	TurnBase(id, flags),
	ExclusiveTurnManager::ExclusiveTurn(flags)
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
/// ELMEngine
////////////////////////////////////////////////////////////////////////////////////////
void ELMEngine::OnNodeCreate(Node& node)
{
	if (node.IsInputNode())
		inputNodes_.insert(&node);
}

void ELMEngine::OnNodeDestroy(Node& node)
{
	// NOTE: Called from dtor, vtables have already been changed and IsInputNode will never return true
	// maybe change this
	inputNodes_.erase(&node);
}

void ELMEngine::OnNodeAttach(Node& node, Node& parent)
{
	parent.Successors.Add(node);
}

void ELMEngine::OnNodeDetach(Node& node, Node& parent)
{
	parent.Successors.Remove(node);
}

void ELMEngine::OnTurnAdmissionStart(Turn& turn)
{
	StartTurn(turn);
}

void ELMEngine::OnTurnAdmissionEnd(Turn& turn)
{
	turn.RunMergedInputs();
}

void ELMEngine::OnTurnInputChange(Node& node, Turn& turn)
{
	node.LastTurnId = turn.Id();
}

void ELMEngine::OnTurnPropagate(Turn& turn)
{
	for (auto* node : inputNodes_)
		nudgeChildren(*node, node->LastTurnId == turn.Id(), turn);

	tasks_.wait();
}

void ELMEngine::OnTurnEnd(Turn& turn)
{
	EndTurn(turn);
}

void ELMEngine::OnNodePulse(Node& node, Turn& turn)
{
	nudgeChildren(node, true, turn);
}

void ELMEngine::OnNodeIdlePulse(Node& node, Turn& turn)
{
	nudgeChildren(node, false, turn);
}

void ELMEngine::OnNodeShift(Node& node, Node& oldParent, Node& newParent, Turn& turn)
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

void ELMEngine::processChild(Node& node, bool update, Turn& turn)
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

void ELMEngine::nudgeChildren(Node& node, bool update, Turn& turn)
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
		tasks_.run(std::bind(&ELMEngine::processChild, this, std::ref(*succ), update, std::ref(turn)));
	}

	node.LastTurnId = turn.Id();	
}// ~node.ShiftMutex

} // ~namespace elm
REACT_IMPL_END