#include "react/propagation/FloodingEngine.h"

////////////////////////////////////////////////////////////////////////////////////////
REACT_IMPL_BEGIN_
namespace flooding {

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
	isScheduled_(false),
	isProcessing_(false),
	shouldReprocess_(false)
{
}

bool Node::MarkForSchedule()
{
	if (IsOutputNode())
		return true;
	
	return isScheduled_.exchange(true, std::memory_order_relaxed) != true;
}

bool Node::Evaluate(Turn& turn)
{
	isScheduled_.store(false, std::memory_order_relaxed);

	{// mutex_
		EvalMutexT::scoped_lock	lock(mutex_);

		if (isProcessing_)
		{
			// Already processing
			// Tell task which is processing it to reprocess after it's done
			shouldReprocess_ = true;
			return false;
		}
		else
		{
			isProcessing_ = true;
		}
	}// ~mutex_

	Tick(&turn);

	bool result = false;

	{// mutex_
		EvalMutexT::scoped_lock	lock(mutex_);

		isProcessing_ = false;

		result = shouldReprocess_;
		shouldReprocess_ = false;
	}// ~mutex_

	return result;
}

////////////////////////////////////////////////////////////////////////////////////////
/// FloodingEngine
////////////////////////////////////////////////////////////////////////////////////////
void FloodingEngine::OnNodeAttach(Node& node, Node& parent)
{
	parent.Successors.Add(node);
}

void FloodingEngine::OnNodeDetach(Node& node, Node& parent)
{
	parent.Successors.Remove(node);
}

void FloodingEngine::OnTurnAdmissionStart(Turn& turn)
{
	StartTurn(turn);
}

void FloodingEngine::OnTurnAdmissionEnd(Turn& turn)
{
	turn.RunMergedInputs();
}

void FloodingEngine::OnTurnInputChange(Node& node, Turn& turn)
{
	changedInputs_.push_back(&node);
}

void FloodingEngine::OnTurnPropagate(Turn& turn)
{
	// Non-input nodes
	for (auto* node : changedInputs_)
		pulse(*node, turn);
	tasks_.wait();

	// Input nodes
	for (auto node : outputNodes_)
		tasks_.run(std::bind(&Node::Tick, node, &turn));

	tasks_.wait();

	outputNodes_.clear();
	changedInputs_.clear();
}

void FloodingEngine::OnTurnEnd(Turn& turn)
{
	EndTurn(turn);
}

void FloodingEngine::OnNodePulse(Node& node, Turn& turn)
{
	pulse(node, turn);
}

void FloodingEngine::OnNodeShift(Node& node, Node& oldParent, Node& newParent, Turn& turn)
{
	{// oldParent.ShiftMutex
		NodeShiftMutexT::scoped_lock	lock(oldParent.ShiftMutex);

		oldParent.Successors.Remove(node);
	}// ~oldParent.ShiftMutex
	
	{// newParent.ShiftMutex
		NodeShiftMutexT::scoped_lock	lock(newParent.ShiftMutex);
	
		newParent.Successors.Add(node);
	}// ~newParent.ShiftMutex

	// Called from Tick, so we already have exclusive access to the node.
	// Just tick again to recalc the value.
	node.Tick(&turn);
}

void FloodingEngine::pulse(Node& node, Turn& turn)
{// node.ShiftMutex
	NodeShiftMutexT::scoped_lock	lock(node.ShiftMutex);

	for (auto* succ : node.Successors)
		if (succ->MarkForSchedule())
			tasks_.run(std::bind(&FloodingEngine::process, this, std::ref(*succ), std::ref(turn)));
}// ~node.ShiftMutex

void FloodingEngine::process(Node& node, Turn& turn)
{
	if (! node.IsOutputNode())
	{
		bool shouldRepeat = false;
		do
			shouldRepeat = node.Evaluate(turn);
		while (shouldRepeat);
	}
	else
	{// outputMutex_
		OutputMutexT::scoped_lock	lock(outputMutex_);

		outputNodes_.insert(&node);		
	}// ~outputMutex_
}

} // ~namespace flooding
REACT_IMPL_END_