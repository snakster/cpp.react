#include "react/propagation/SourceSetEngine.h"

#include "tbb/task_group.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace react {
namespace source_set_impl {

tbb::task_group		tasks;

////////////////////////////////////////////////////////////////////////////////////////
/// SourceSetTurn
////////////////////////////////////////////////////////////////////////////////////////
SourceSetTurn::SourceSetTurn(TransactionData<SourceSetTurn>& transactionData) :
	TurnBase(transactionData),
	ExclusiveSequentialTransaction(transactionData.Input())
{
}

void SourceSetTurn::AddSourceId(ObjectId id)
{
	sources_.Insert(id);
}

////////////////////////////////////////////////////////////////////////////////////////
/// SourceSetNode
////////////////////////////////////////////////////////////////////////////////////////
SourceSetNode::SourceSetNode() :
	curTurnId_(UINT_MAX),
	tickThreshold_(0),
	flags_(0)
{
}

void SourceSetNode::AddSourceId(ObjectId id)
{
	sources_.Insert(id);
}

void SourceSetNode::AttachSuccessor(SourceSetNode& node)
{
	successors_.Add(node);
	node.predecessors_.Add(*this);

	node.sources_.Insert(sources_);
}

void SourceSetNode::DetachSuccessor(SourceSetNode& node)
{
	successors_.Remove(node);
	node.predecessors_.Remove(*this);

	node.invalidateSources();
}

void SourceSetNode::Destroy()
{
	auto predIt = predecessors_.begin();
	while (predIt != predecessors_.end())
	{
		(*predIt)->DetachSuccessor(*this);
		predIt = predecessors_.begin();
	}

	auto succIt = successors_.begin();
	while (succIt != successors_.end())
	{
		DetachSuccessor(**succIt);
		succIt = successors_.begin();
	}
}

void SourceSetNode::Pulse(SourceSetTurn& turn, bool updated)
{
	bool invalidate = flags_ & kFlag_Invaliated;
	flags_ &= ~(kFlag_Invaliated | kFlag_Updated | kFlag_Visited);

	// shiftMutex_
	{
		ShiftMutexT::scoped_lock	lock(shiftMutex_);

		curTurnId_ = turn.Id();

		for (auto succ : successors_)
			tasks.run(std::bind(&SourceSetNode::Nudge, succ, std::ref(turn), updated, invalidate));
	}
	// ~shiftMutex_
}

bool SourceSetNode::IsDependency(SourceSetTurn& turn)
{
	return turn.Sources().IntersectsWith(sources_);
}

bool SourceSetNode::CheckCurrentTurn(SourceSetTurn& turn)
{
	return curTurnId_ == turn.Id();
}

void SourceSetNode::Nudge(SourceSetTurn& turn, bool update, bool invalidate)
{
	bool shouldTick = false;

	// nudgeMutex_
	{
		NudgeMutexT::scoped_lock	lock(nudgeMutex_);

		if (update)
			flags_ |= kFlag_Updated;

		if (invalidate)
			flags_ |= kFlag_Invaliated;

		// First nudge initializes threshold counter for this turn
		if (! (flags_ & kFlag_Visited))
		{
			flags_ |= kFlag_Visited;
			tickThreshold_ = 0;

			// Count unprocessed dependencies
			for (auto pred : predecessors_)
				if (pred->IsDependency(turn))
					++tickThreshold_;
		}

		// Wait for other predecessors?
		if (--tickThreshold_ > 0)
			return;
	}
	// ~nudgeMutex_

	if (flags_ & kFlag_Updated)
		shouldTick = true;

	if (flags_ & kFlag_Invaliated)
		invalidateSources();

	flags_ &= ~(kFlag_Visited | kFlag_Updated);
	if (IsOutputNode())
			flags_ &= ~kFlag_Invaliated;

	if (shouldTick)
		Tick(&turn);
	else
		Pulse(turn, false);
}

void SourceSetNode::Shift(SourceSetNode& oldParent, SourceSetNode& newParent, SourceSetTurn& turn)
{
	bool shouldTick = false;

	// oldParent.shiftMutex_
	{
		ShiftMutexT::scoped_lock	lock(oldParent.shiftMutex_);

		oldParent.DetachSuccessor(*this);
	}
	// ~oldParent.topoMutex_

	// newParent.shiftMutex_
	{
		ShiftMutexT::scoped_lock	lock(newParent.shiftMutex_);

		newParent.AttachSuccessor(*this);

		flags_ |= kFlag_Invaliated;

		// Has new parent been processed yet?
		if (newParent.IsDependency(turn) && !newParent.CheckCurrentTurn(turn))
		{
			tickThreshold_ = 1;
			flags_ |= kFlag_Visited | kFlag_Updated;
		}
		else
		{
			shouldTick = true;
		}
	}
	// ~newParent.topoMutex_

	// Re-tick?
	if (shouldTick)
		Tick(&turn);
}

void SourceSetNode::invalidateSources()
{
	// Recalc union
	sources_.Clear();

	for (auto pred : predecessors_)
		sources_.Insert(pred->sources_);		
}

////////////////////////////////////////////////////////////////////////////////////////
/// SourceSetEngine
////////////////////////////////////////////////////////////////////////////////////////
void SourceSetEngine::OnNodeCreate(SourceSetNode& node)
{
	if (node.IsInputNode())
		node.AddSourceId(GetObjectId(node));
}

void SourceSetEngine::OnNodeAttach(SourceSetNode& node, SourceSetNode& parent)
{
	parent.AttachSuccessor(node);
}

void SourceSetEngine::OnNodeDetach(SourceSetNode& node, SourceSetNode& parent)
{
	parent.DetachSuccessor(node);
}

void SourceSetEngine::OnNodeDestroy(SourceSetNode& node)
{
	node.Destroy();
}

void SourceSetEngine::OnTransactionCommit(TransactionData<SourceSetTurn>& transaction)
{
	SourceSetTurn turn(transaction);

	if (! BeginTransaction(turn))
		return;

	transaction.Input().RunAdmission(turn);

	transaction.Input().RunPropagation(turn);
	tasks.wait();

	EndTransaction(turn);
}

void SourceSetEngine::OnInputNodeAdmission(SourceSetNode& node, SourceSetTurn& turn)
{
	turn.AddSourceId(GetObjectId(node));
}

void SourceSetEngine::OnNodePulse(SourceSetNode& node, SourceSetTurn& turn)
{
	node.Pulse(turn, true);
}

void SourceSetEngine::OnNodeIdlePulse(SourceSetNode& node, SourceSetTurn& turn)
{
	node.Pulse(turn, false);
}

void SourceSetEngine::OnNodeShift(SourceSetNode& node, SourceSetNode& oldParent, SourceSetNode& newParent, SourceSetTurn& turn)
{
	node.Shift(oldParent, newParent, turn);
}

} // ~namespace react::source_set_impl
} // ~namespace react