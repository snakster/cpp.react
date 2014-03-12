
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "react/propagation/SourceSetEngine.h"

#include "tbb/task_group.h"

////////////////////////////////////////////////////////////////////////////////////////
REACT_IMPL_BEGIN
namespace sourceset {

// Todo
tbb::task_group		tasks;

////////////////////////////////////////////////////////////////////////////////////////
/// Turn
////////////////////////////////////////////////////////////////////////////////////////
Turn::Turn(TurnIdT id, TurnFlagsT flags) :
	TurnBase(id, flags)
{
}

void Turn::AddSourceId(ObjectId id)
{
	sources_.Insert(id);
}

////////////////////////////////////////////////////////////////////////////////////////
/// Node
////////////////////////////////////////////////////////////////////////////////////////
Node::Node() :
	curTurnId_(UINT_MAX),
	tickThreshold_(0),
	flags_(0)
{
}

void Node::AddSourceId(ObjectId id)
{
	sources_.Insert(id);
}

void Node::AttachSuccessor(Node& node)
{
	successors_.Add(node);
	node.predecessors_.Add(*this);

	node.sources_.Insert(sources_);
}

void Node::DetachSuccessor(Node& node)
{
	successors_.Remove(node);
	node.predecessors_.Remove(*this);

	node.invalidateSources();
}

void Node::Destroy()
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

void Node::Pulse(Turn& turn, bool updated)
{
	bool invalidate = (flags_ & kFlag_Invaliated) != 0;
	flags_ &= ~(kFlag_Invaliated | kFlag_Updated | kFlag_Visited);

	// shiftMutex_
	{
		ShiftMutexT::scoped_lock	lock(shiftMutex_);

		curTurnId_ = turn.Id();

		for (auto succ : successors_)
			tasks.run(std::bind(&Node::Nudge, succ, std::ref(turn), updated, invalidate));
	}
	// ~shiftMutex_
}

bool Node::IsDependency(Turn& turn)
{
	return turn.Sources().IntersectsWith(sources_);
}

bool Node::CheckCurrentTurn(Turn& turn)
{
	return curTurnId_ == turn.Id();
}

void Node::Nudge(Turn& turn, bool update, bool invalidate)
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

void Node::Shift(Node& oldParent, Node& newParent, Turn& turn)
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

void Node::invalidateSources()
{
	// Recalc union
	sources_.Clear();

	for (auto pred : predecessors_)
		sources_.Insert(pred->sources_);		
}

////////////////////////////////////////////////////////////////////////////////////////
/// SourceSetEngine
////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurn>
void EngineBase<TTurn>::OnNodeCreate(Node& node)
{
	if (node.IsInputNode())
		node.AddSourceId(GetObjectId(node));
}

template <typename TTurn>
void EngineBase<TTurn>::OnNodeAttach(Node& node, Node& parent)
{
	parent.AttachSuccessor(node);
}

template <typename TTurn>
void EngineBase<TTurn>::OnNodeDetach(Node& node, Node& parent)
{
	parent.DetachSuccessor(node);
}

template <typename TTurn>
void EngineBase<TTurn>::OnNodeDestroy(Node& node)
{
	node.Destroy();
}

template <typename TTurn>
void EngineBase<TTurn>::OnTurnInputChange(Node& node, TTurn& turn)
{
	turn.AddSourceId(GetObjectId(node));
	changedInputs_.push_back(&node);
}

template <typename TTurn>
void EngineBase<TTurn>::OnTurnPropagate(TTurn& turn)
{
	for (auto* node : changedInputs_)
		node->Pulse(turn, true);
	tasks.wait();

	changedInputs_.clear();
}

template <typename TTurn>
void EngineBase<TTurn>::OnNodePulse(Node& node, TTurn& turn)
{
	node.Pulse(turn, true);
}

template <typename TTurn>
void EngineBase<TTurn>::OnNodeIdlePulse(Node& node, TTurn& turn)
{
	node.Pulse(turn, false);
}

template <typename TTurn>
void EngineBase<TTurn>::OnNodeShift(Node& node, Node& oldParent, Node& newParent, TTurn& turn)
{
	node.Shift(oldParent, newParent, turn);
}

// Explicit instantiation
template class EngineBase<Turn>;
template class EngineBase<DefaultQueueableTurn<Turn>>;

} // ~namespace sourceset
REACT_IMPL_END