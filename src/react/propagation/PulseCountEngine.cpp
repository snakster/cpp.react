#include "react/propagation/PulseCountEngine.h"

#include <cstdint>

#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"
#include "tbb/tick_count.h"

#include "react/common/Types.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace react {
namespace pulse_count_impl {

////////////////////////////////////////////////////////////////////////////////////////
/// Turn
////////////////////////////////////////////////////////////////////////////////////////
Turn::Turn(TransactionData<Turn>& transactionData) :
	TurnBase(transactionData),
	ExclusiveSequentialTransaction(transactionData.Input())
{
}

////////////////////////////////////////////////////////////////////////////////////////
/// Node
////////////////////////////////////////////////////////////////////////////////////////
Node::Node() :
	TickThreshold(0),
	ShouldUpdate(false),
	Marked(false)
{
}

////////////////////////////////////////////////////////////////////////////////////////
/// PulseCountEngine
////////////////////////////////////////////////////////////////////////////////////////
void PulseCountEngine::OnNodeAttach(Node& node, Node& parent)
{
	parent.Successors.Add(node);
}

void PulseCountEngine::OnNodeDetach(Node& node, Node& parent)
{
	parent.Successors.Remove(node);
}

void PulseCountEngine::OnTransactionCommit(TransactionData<Turn>& transaction)
{
	Turn turn(transaction);

	if (! BeginTransaction(turn))
		return;

	//auto t0 = tbb::tick_count::now();
	transaction.Input().RunAdmission(turn);
	tasks_.wait();
	//auto t1 = tbb::tick_count::now();
	//double d = (t1 - t0).seconds();
	//printf("Time %f\n", d);

	transaction.Input().RunPropagation(turn);
	tasks_.wait();

	EndTransaction(turn);
}

void PulseCountEngine::OnInputNodeAdmission(Node& node, Turn& turn)
{
	tasks_.run(std::bind(&PulseCountEngine::initTurn, this, std::ref(node), std::ref(turn)));
}

void PulseCountEngine::OnNodePulse(Node& node, Turn& turn)
{
	nudgeChildren(node, true, turn);
}

void PulseCountEngine::OnNodeIdlePulse(Node& node, Turn& turn)
{
	nudgeChildren(node, false, turn);
}

void PulseCountEngine::OnNodeShift(Node& node, Node& oldParent, Node& newParent, Turn& turn)
{
	bool shouldTick = false;

	// oldParent.ShiftMutex (write)
	{
		NodeShiftMutexT::scoped_lock	lock(oldParent.ShiftMutex, true);

		oldParent.Successors.Remove(node);
	}
	// ~oldParent.ShiftMutex (write)

	// newParent.ShiftMutex (write)
	{
		NodeShiftMutexT::scoped_lock	lock(newParent.ShiftMutex, true);
		
		newParent.Successors.Add(node);

		// Has already been ticked & nudged its neighbors? (Note: Input nodes are always ready)
		if (! newParent.Marked)
		{
			shouldTick = true;
		}
		else
		{
			node.TickThreshold = 1;
			node.ShouldUpdate = true;
		}
	}
	// ~newParent.ShiftMutex (write)

	if (shouldTick)
		node.Tick(&turn);
}

void PulseCountEngine::initTurn(Node& node, Turn& turn)
{
	for (auto* succ : node.Successors)
	{
		succ->TickThreshold.fetch_add(1, std::memory_order_relaxed);

		if (!succ->Marked.exchange(true))
			tasks_.run(std::bind(&PulseCountEngine::initTurn, this, std::ref(*succ), std::ref(turn)));
	}
}

void PulseCountEngine::processChild(Node& node, bool update, Turn& turn)
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

void PulseCountEngine::nudgeChildren(Node& node, bool update, Turn& turn)
{
	// node.ShiftMutex (read)
	{
		NodeShiftMutexT::scoped_lock	lock(node.ShiftMutex);

		// Select first child as next node, dispatch tasks for rest
		for (auto* succ : node.Successors)
		{
			if (update)
				succ->ShouldUpdate = true;

			// Delay tick?
			if (succ->TickThreshold-- > 1)
				continue;

			tasks_.run(std::bind(&PulseCountEngine::processChild, this, std::ref(*succ), update, std::ref(turn)));
		}

		node.Marked = false;
	}
	// ~node.ShiftMutex (read)
}

} // ~namespace react::pulse_count_impl
} // ~namespace react