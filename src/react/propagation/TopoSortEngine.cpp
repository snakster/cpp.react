#include "react/propagation/TopoSortEngine.h"

////////////////////////////////////////////////////////////////////////////////////////
REACT_IMPL_BEGIN
namespace toposort {

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
template <typename TNode, typename TTurn>
void EngineBase<TNode,TTurn>::OnNodeAttach(TNode& node, TNode& parent)
{
	parent.Successors.Add(node);

	if (node.Level <= parent.Level)
		node.Level = parent.Level + 1;
}

template <typename TNode, typename TTurn>
void EngineBase<TNode,TTurn>::OnNodeDetach(TNode& node, TNode& parent)
{
	parent.Successors.Remove(node);
}

template <typename TNode, typename TTurn>
void EngineBase<TNode,TTurn>::OnTurnInputChange(TNode& node, TTurn& turn)
{
	processChildren(node, turn);
}

template <typename TNode, typename TTurn>
void EngineBase<TNode,TTurn>::OnNodePulse(TNode& node, TTurn& turn)
{
	processChildren(node, turn);
}

template <typename TNode, typename TTurn>
void EngineBase<TNode,TTurn>::invalidateSuccessors(TNode& node)
{
	for (auto* succ : node.Successors)
	{
		if (succ->NewLevel <= node.Level)
			succ->NewLevel = node.Level + 1;
	}
}


// Explicit instantiation
template class EngineBase<SeqNode,Turn>;
template class EngineBase<ParNode,Turn>;
template class EngineBase<SeqNode,DefaultQueueableTurn<Turn>>;
template class EngineBase<ParNode,DefaultQueueableTurn<Turn>>;

////////////////////////////////////////////////////////////////////////////////////////
/// SeqEngineBase
////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurn>
void SeqEngineBase<TTurn>::OnTurnPropagate(TTurn& turn)
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
void SeqEngineBase<TTurn>::OnNodeShift(SeqNode& node, SeqNode& oldParent, SeqNode& newParent, TTurn& turn)
{
	OnNodeDetach(node, oldParent);
	OnNodeAttach(node, newParent);
	
	invalidateSuccessors(node);

	// Re-schedule this node
	node.Queued = true;
	scheduledNodes_.Push(&node);
}

template <typename TTurn>
void SeqEngineBase<TTurn>::processChildren(SeqNode& node, TTurn& turn)
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

// Explicit instantiation
template class SeqEngineBase<Turn>;
template class SeqEngineBase<DefaultQueueableTurn<Turn>>;

////////////////////////////////////////////////////////////////////////////////////////
/// SeqEngineBase
////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurn>
void ParEngineBase<TTurn>::OnTurnPropagate(TTurn& turn)
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
			tasks_.run(std::bind(&ParNode::Tick, curNode, &turn));

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
void ParEngineBase<TTurn>::OnNodeShift(ParNode& node, ParNode& oldParent, ParNode& newParent, TTurn& turn)
{
	// Invalidate may have to wait for other transactions to leave the target interval.
	// Waiting in this task would block the worker thread, so we defer the request to the main
	// transaction loop (see applyInvalidate).
	ShiftRequestData data{ &node, &oldParent, &newParent };
	shiftRequests_.push_back(data);
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
void ParEngineBase<TTurn>::applyShift(ParNode& node, ParNode& oldParent, ParNode& newParent, TTurn& turn)
{
	OnNodeDetach(node, oldParent);
	OnNodeAttach(node, newParent);

	invalidateSuccessors(node);

	// Re-schedule this node
	node.Collected = true;
	collectBuffer_.push_back(&node);
}

template <typename TTurn>
void ParEngineBase<TTurn>::processChildren(ParNode& node, TTurn& turn)
{
	// Add children to queue
	for (auto* succ : node.Successors)
	{
		if (!succ->Collected.exchange(true))
			collectBuffer_.push_back(succ);
	}
}

// Explicit instantiation
template class ParEngineBase<Turn>;
template class ParEngineBase<DefaultQueueableTurn<Turn>>;

} // ~namespace toposort
REACT_IMPL_END