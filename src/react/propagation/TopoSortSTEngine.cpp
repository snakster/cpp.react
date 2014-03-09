#include "react/propagation/TopoSortSTEngine.h"

////////////////////////////////////////////////////////////////////////////////////////
REACT_IMPL_BEGIN
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
/// Engine
////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurn>
void EngineBase<TTurn>::OnNodeAttach(Node& node, Node& parent)
{
	parent.Successors.Add(node);

	if (node.Level <= parent.Level)
		node.Level = parent.Level + 1;
}

template <typename TTurn>
void EngineBase<TTurn>::OnNodeDetach(Node& node, Node& parent)
{
	parent.Successors.Remove(node);
}

template <typename TTurn>
void EngineBase<TTurn>::OnTurnInputChange(Node& node, TTurn& turn)
{
	processChildren(node, turn);
}

template <typename TTurn>
void EngineBase<TTurn>::OnTurnPropagate(Turn& turn)
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

template <typename TTurn>
void EngineBase<TTurn>::OnNodePulse(Node& node, TTurn& turn)
{
	processChildren(node, turn);
}

template <typename TTurn>
void EngineBase<TTurn>::OnNodeShift(Node& node, Node& oldParent, Node& newParent, TTurn& turn)
{
	OnNodeDetach(node, oldParent);
	OnNodeAttach(node, newParent);
	
	invalidateSuccessors(node);

	// Re-schedule this node
	node.Queued = true;
	scheduledNodes_.Push(&node);
}

template <typename TTurn>
void EngineBase<TTurn>::processChildren(Node& node, TTurn& turn)
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

template <typename TTurn>
void EngineBase<TTurn>::invalidateSuccessors(Node& node)
{
	for (auto* succ : node.Successors)
	{
		if (succ->NewLevel <= node.Level)
			succ->NewLevel = node.Level + 1;
	}
}

// Explicit instantiation
template class EngineBase<Turn>;
template class EngineBase<DefaultQueueableTurn<Turn>>;

} // ~namespace toposort_st
REACT_IMPL_END