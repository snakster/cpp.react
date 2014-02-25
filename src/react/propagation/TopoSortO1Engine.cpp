#include "react/propagation/TopoSortO1Engine.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace react {
namespace topo_sort_o1_impl {

////////////////////////////////////////////////////////////////////////////////////////
/// Node
////////////////////////////////////////////////////////////////////////////////////////
Node::Node() :
	Level(0),
	Collected(false),
	Invalidated(false)
{
}

////////////////////////////////////////////////////////////////////////////////////////
/// Turn
////////////////////////////////////////////////////////////////////////////////////////
Turn::Turn(TransactionData<Turn>& transactionData) :
	TurnBase(transactionData),
	input_(transactionData.Input()),
	currentLevel_(-1),
	minLevel_(-1),
	maxLevel_(INT_MAX),
	predecessor_(nullptr),
	successor_(nullptr)
{
}

bool Turn::TryMerge(Turn& other)
{
	std::unique_lock<std::mutex> lock(advanceMutex_);

	// Already started?
	if (currentLevel_ > -1)
		return false;

	input_.Merge(other.input_);
	mergedTurns_.push_back(&other);

	return true;
}

bool Turn::AdvanceLevel()
{
	std::unique_lock<std::mutex> lock(advanceMutex_);

	while (currentLevel_ + 1 > maxLevel_)
		advanceCondition_.wait(lock);

	// Remove the intervals we're going to leave
	auto it = levelIntervals_.begin();
	while (it != levelIntervals_.end())
	{
		if (it->second <= currentLevel_)
			it = levelIntervals_.erase(it);
		else
			++it;
	}

	// Add new interval for current level
	if (currentLevel_ < curUpperBound_)
		levelIntervals_.emplace(currentLevel_, curUpperBound_);

	currentLevel_++;

	curUpperBound_ = currentLevel_;
	
	// Min level is the minimum over all interval lower bounds
	int newMinLevel = levelIntervals_.begin() != levelIntervals_.end() ? levelIntervals_.begin()->first : -1;

	if (minLevel_ != newMinLevel)
	{
		minLevel_ = newMinLevel;
		return true;
	}
	else
	{
		return false;
	}
}

void Turn::SetMaxLevel(int level)
{
	std::unique_lock<std::mutex> lock(advanceMutex_);

	maxLevel_ = level;
	advanceCondition_.notify_all();
}

void Turn::WaitForMaxLevel(int targetLevel)
{
	std::unique_lock<std::mutex> lock(advanceMutex_);

	while (targetLevel < maxLevel_)
		advanceCondition_.wait(lock);
}

void Turn::Append(Turn* turn)
{
	successor_ = turn;

	if (turn)
		turn->predecessor_ = this;

	UpdateSuccessor();
}

void Turn::UpdateSuccessor()
{
	if (successor_)
		successor_->SetMaxLevel(minLevel_ - 1);
}

void Turn::Remove()
{
	if (predecessor_)
	{
		predecessor_->Append(successor_);
	}
	else if (successor_)
	{
		successor_->SetMaxLevel(INT_MAX);
		successor_->predecessor_ = nullptr;
	}

	// Notify merged turns.
	// Any level > -1 is fine since they won't actually advance.
	for (auto p : mergedTurns_)
		p->SetMaxLevel(0);
}

void Turn::AdjustUpperBound(int level)
{
	if (curUpperBound_ < level)
		curUpperBound_ = level;
}

////////////////////////////////////////////////////////////////////////////////////////
/// TopoSortO1Engine
////////////////////////////////////////////////////////////////////////////////////////
TopoSortO1Engine::TopoSortO1Engine() :
	lastTurn_(nullptr)
{
}

void TopoSortO1Engine::OnNodeAttach(Node& node, Node& parent)
{
	parent.Successors.Add(node);

	if (node.Level <= parent.Level)
		node.Level = parent.Level + 1;

	if (node.IsDynamicNode())
	{
		dynamicNodes_.insert(&node);

		if (maxDynamicLevel_ < node.Level)
			maxDynamicLevel_ = node.Level;
	}
}

void TopoSortO1Engine::OnNodeDetach(Node& node, Node& parent)
{
	parent.Successors.Remove(node);

	// Recalc maxdynamiclevel?
	if (node.IsDynamicNode())
	{
		dynamicNodes_.erase(&node);
		if (maxDynamicLevel_ == node.Level)
		{
			maxDynamicLevel_ = 0;
			for (auto e : dynamicNodes_)
				if (maxDynamicLevel_ < e->Level)
					maxDynamicLevel_ = e->Level;
		}
	}
}

void TopoSortO1Engine::OnTransactionCommit(TransactionData<Turn>& transaction)
{
	Turn turn(transaction);

	bool allowMerging = transaction.Input().Flags() & allow_transaction_merging;

	// False, if merged instead of added
	if (! addTurn(turn, allowMerging))
	{
		turn.WaitForMaxLevel(0);
		// Note: Merged turns don't have to be removed with removeTurn
		return;
	}

	advanceTurn(turn);

	// TODO: Find a better place for this?
	if (maxDynamicLevel_ > 0)
		turn.AdjustUpperBound(maxDynamicLevel_);

	transaction.Input().RunAdmission(turn);

	transaction.Input().RunPropagation(turn);

	bool repeatLevel = false;

	while (!turn.CollectBuffer.empty() || !turn.ScheduledNodes.Empty())
	{
		// Merge thread-safe buffer of nodes that pulsed during last turn into queue
		for (auto node : turn.CollectBuffer)
		{
			turn.AdjustUpperBound(node->Level);
			turn.ScheduledNodes.Push(node);
		}
		turn.CollectBuffer.clear();

		// Advance
		if (!repeatLevel)
			advanceTurn(turn);
		else
			repeatLevel = false;

		// Pop all nodes of current level and start processing them in parallel
		while (!turn.ScheduledNodes.Empty())
		{
			auto node = turn.ScheduledNodes.Top();

			if (node->Level > turn.CurrentLevel())
				break;

			turn.ScheduledNodes.Pop();

#ifdef _DEBUG
			REACT_ASSERT(turn.CurrentLevel() == node->Level, "Processed node with level != curLevel");
#endif

			node->Collected = false;

			// Tick -> if changed: OnNodePulse -> adds child nodes to the queue
			turn.Tasks.run(std::bind(&Node::Tick, node, &turn));
		}

		// Wait for tasks of current level
		turn.Tasks.wait();

		if (turn.InvalidateRequests.size() > 0)
		{
			for (auto req : turn.InvalidateRequests)
				applyInvalidate(*req.ShiftingNode, *req.OldParent, *req.NewParent, turn);
			turn.InvalidateRequests.clear();

			repeatLevel = true;
		}
	}

	removeTurn(turn);
}

void TopoSortO1Engine::OnNodePulse(Node& node, Turn& turn)
{
	processChildren(node, turn);
}

void TopoSortO1Engine::OnNodeShift(Node& node, Node& oldParent, Node& newParent, Turn& turn)
{
	// Invalidate may have to wait for other transactions to leave the target interval.
	// Waiting in this task would block the worker thread, so we defer the request to the main
	// transaction loop (see applyInvalidate).
	InvalidateData d = {&node, &oldParent, &newParent};
	turn.InvalidateRequests.push_back(d);
}

void TopoSortO1Engine::applyInvalidate(Node& node, Node& oldParent, Node& newParent, Turn& turn)
{
	turn.WaitForMaxLevel(INT_MAX);

	OnNodeDetach(node, oldParent);
	OnNodeAttach(node, newParent);
	
	recalculateLevels(node);

	turn.ScheduledNodes.Invalidate();

	// Re-schedule this node
	turn.CollectBuffer.push_back(&node);
}

void TopoSortO1Engine::processChildren(Node& node, Turn& turn)
{
	// Add children to queue
	for (auto* succ : node.Successors)
	{
		if (!succ->Collected.exchange(true))
			turn.CollectBuffer.push_back(succ);
	}
}

void TopoSortO1Engine::recalculateLevels(Node& node)
{
	// So far, the levels can only grow
	for (auto* succ : node.Successors)
	{
		if (succ->Level <= node.Level)
		{
			succ->Level = node.Level + 1;

			if (succ->IsDynamicNode() && maxDynamicLevel_ < succ->Level)
				maxDynamicLevel_ = succ->Level;

			recalculateLevels(*succ);
		}
	}
}

bool TopoSortO1Engine::addTurn(Turn& turn, bool allowMerging)
{
	bool merged = false;

	// sequenceMutex_ (write)
	{
		SeqMutex::scoped_lock	lock(sequenceMutex_, true);

		if (lastTurn_)
		{
			if (allowMerging)
				merged = lastTurn_->TryMerge(turn);

			if (!merged)
				lastTurn_->Append(&turn);
		}
		
		if (!merged)
			lastTurn_ = &turn;
	}
	// ~sequenceMutex_

	return !merged;
}

void TopoSortO1Engine::removeTurn(Turn& turn)
{
	// sequenceMutex_ (write)
	{
		SeqMutex::scoped_lock	lock(sequenceMutex_, true);

		turn.Remove();

		if (lastTurn_ == &turn)
			lastTurn_ = nullptr;
	}
	// ~sequenceMutex_

}

void TopoSortO1Engine::advanceTurn(Turn& turn)
{
	if (!turn.AdvanceLevel())
		return;

	// sequenceMutex_ (read)
	{
		SeqMutex::scoped_lock	lock(sequenceMutex_, false);

		turn.UpdateSuccessor();
	}
	// ~sequenceMutex_
}

} // ~namespace react::topo_sort_o1_impl
} // ~namespace react
