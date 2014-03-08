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
	TurnBase(id, flags),
	ExclusiveTurnManager::ExclusiveTurn(flags)
{
}

////////////////////////////////////////////////////////////////////////////////////////
/// TopoSortEngine
////////////////////////////////////////////////////////////////////////////////////////
TopoSortEngine::TopoSortEngine()
{
}

void TopoSortEngine::OnNodeAttach(Node& node, Node& parent)
{
	parent.Successors.Add(node);

	if (node.Level <= parent.Level)
		node.Level = parent.Level + 1;
}

void TopoSortEngine::OnNodeDetach(Node& node, Node& parent)
{
	parent.Successors.Remove(node);
}

void TopoSortEngine::OnTurnAdmissionStart(Turn& turn)
{
	StartTurn(turn);
}

void TopoSortEngine::OnTurnAdmissionEnd(Turn& turn)
{
	turn.RunMergedInputs();
}

void TopoSortEngine::OnTurnInputChange(Node& node, Turn& turn)
{
	processChildren(node, turn);
}

void TopoSortEngine::OnTurnPropagate(Turn& turn)
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

void TopoSortEngine::OnTurnEnd(Turn& turn)
{
	EndTurn(turn);
}

void TopoSortEngine::OnNodePulse(Node& node, Turn& turn)
{
	processChildren(node, turn);
}

void TopoSortEngine::OnNodeShift(Node& node, Node& oldParent, Node& newParent, Turn& turn)
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

void TopoSortEngine::applyShift(Node& node, Node& oldParent, Node& newParent, Turn& turn)
{
	OnNodeDetach(node, oldParent);
	OnNodeAttach(node, newParent);

	invalidateSuccessors(node);
	//recalculateLevels(node);

	//scheduledNodes_.Invalidate();

	// Re-schedule this node
	node.Collected = true;
	collectBuffer_.push_back(&node);
}

void TopoSortEngine::processChildren(Node& node, Turn& turn)
{
	// Add children to queue
	for (auto* succ : node.Successors)
	{
		if (!succ->Collected.exchange(true))
			collectBuffer_.push_back(succ);
	}
}

void TopoSortEngine::invalidateSuccessors(Node& node)
{
	for (auto* succ : node.Successors)
	{
		if (succ->NewLevel <= node.Level)
			succ->NewLevel = node.Level + 1;
	}
}

} // ~namespace toposort
REACT_IMPL_END