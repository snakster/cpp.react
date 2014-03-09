#include "react/propagation/TopoSortEngine.h"

////////////////////////////////////////////////////////////////////////////////////////
REACT_IMPL_BEGIN
namespace toposort {

////////////////////////////////////////////////////////////////////////////////////////
/// Node
////////////////////////////////////////////////////////////////////////////////////////
Node::Node() :
	Level(0),
	NewLevel(0),
	Collected(false)
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
/// EngineBase
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
void EngineBase<TTurn>::OnTurnPropagate(TTurn& turn)
{
	while (!collectBuffer_.empty() || !scheduledNodes_.Empty())
	{
		// Merge thread-safe buffer of nodes that pulsed during last turn into queue
		for (auto node : collectBuffer_)
			scheduledNodes_.Push(node);
		collectBuffer_.clear();

		auto curNode = scheduledNodes_.Top();
		auto currentLevel = curNode->Level;

		// Pop all nodes of current level and start processing them in parallel
		do
		{
			scheduledNodes_.Pop();

			if (curNode->Level < curNode->NewLevel)
			{
				curNode->Level = curNode->NewLevel;
				invalidateSuccessors(*curNode);
				scheduledNodes_.Push(curNode);
				break;
			}

			curNode->Collected = false;

			// Tick -> if changed: OnNodePulse -> adds child nodes to the queue
			tasks_.run(std::bind(&Node::Tick, curNode, &turn));

			if (scheduledNodes_.Empty())
				break;

			curNode = scheduledNodes_.Top();
		}
		while (curNode->Level == currentLevel);

		// Wait for tasks of current level
		tasks_.wait();

		if (shiftRequests_.size() > 0)
		{
			for (auto req : shiftRequests_)
				applyShift(*req.ShiftingNode, *req.OldParent, *req.NewParent, turn);
			shiftRequests_.clear();
		}
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
	// Invalidate may have to wait for other transactions to leave the target interval.
	// Waiting in this task would block the worker thread, so we defer the request to the main
	// transaction loop (see applyInvalidate).
	ShiftRequestData d = {&node, &oldParent, &newParent};
	shiftRequests_.push_back(d);
}

//tbb::parallel_for(tbb::blocked_range<NodeVectorT::iterator>(curNodes.begin(), curNodes.end(), 1),
//	[&] (const tbb::blocked_range<NodeVectorT::iterator>& range)
//	{
//		for (const auto& succ : range)
//		{
//			succ->Counter++;

//			if (succ->SetMark(mark))
//				nextNodes.push_back(succ);
//		}
//	}
//);

template <typename TTurn>
void EngineBase<TTurn>::applyShift(Node& node, Node& oldParent, Node& newParent, TTurn& turn)
{
	OnNodeDetach(node, oldParent);
	OnNodeAttach(node, newParent);

	invalidateSuccessors(node);

	// Re-schedule this node
	node.Collected = true;
	collectBuffer_.push_back(&node);
}

template <typename TTurn>
void EngineBase<TTurn>::processChildren(Node& node, TTurn& turn)
{
	// Add children to queue
	for (auto* succ : node.Successors)
	{
		if (!succ->Collected.exchange(true))
			collectBuffer_.push_back(succ);
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

} // ~namespace toposort
REACT_IMPL_END