#include "react/propagation/TopoSortSTEngine.h"

////////////////////////////////////////////////////////////////////////////////////////
REACT_IMPL_BEGIN_
namespace toposort_st {

////////////////////////////////////////////////////////////////////////////////////////
/// Node
////////////////////////////////////////////////////////////////////////////////////////
Node::Node() :
	Level(0),
	NewLevel(0),
	Queued(false)
{
}

////////////////////////////////////////////////////////////////////////////////////////
/// Turn
////////////////////////////////////////////////////////////////////////////////////////
Turn::Turn(TurnIdT id, TurnFlagsT flags) :
	TurnBase(id, flags)
{
}

////////////////////////////////////////////////////////////////////////////////////////
/// TopoSortSTEngine
////////////////////////////////////////////////////////////////////////////////////////
TopoSortSTEngine::TopoSortSTEngine()
{
}

void TopoSortSTEngine::OnNodeAttach(Node& node, Node& parent)
{
	parent.Successors.Add(node);

	if (node.Level <= parent.Level)
		node.Level = parent.Level + 1;
}

void TopoSortSTEngine::OnNodeDetach(Node& node, Node& parent)
{
	parent.Successors.Remove(node);
}

void TopoSortSTEngine::OnTurnInputChange(Node& node, Turn& turn)
{
	processChildren(node, turn);
}

void TopoSortSTEngine::OnTurnPropagate(Turn& turn)
{
	while (!scheduledNodes_.Empty())
	{
		auto node = scheduledNodes_.Top();
		scheduledNodes_.Pop();

		if (node->Level < node->NewLevel)
		{
			node->Level = node->NewLevel;
			invalidateSuccessors(*node);
			scheduledNodes_.Push(node);
			continue;
		}

		node->Queued = false;
		node->Tick(&turn);
	}
}

void TopoSortSTEngine::OnNodePulse(Node& node, Turn& turn)
{
	processChildren(node, turn);
}

void TopoSortSTEngine::OnNodeShift(Node& node, Node& oldParent, Node& newParent, Turn& turn)
{
	OnNodeDetach(node, oldParent);
	OnNodeAttach(node, newParent);
	
	invalidateSuccessors(node);

	// Re-schedule this node
	node.Queued = true;
	scheduledNodes_.Push(&node);
}

void TopoSortSTEngine::processChildren(Node& node, Turn& turn)
{
	// Add children to queue
	for (auto* succ : node.Successors)
	{
		if (!succ->Queued)
		{
			succ->Queued = true;
			scheduledNodes_.Push(succ);
		}
	}
}

void TopoSortSTEngine::invalidateSuccessors(Node& node)
{
	for (auto* succ : node.Successors)
	{
		if (succ->NewLevel <= node.Level)
			succ->NewLevel = node.Level + 1;
	}
}

} // ~namespace toposort_st
REACT_IMPL_END_