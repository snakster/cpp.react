#include "react/propagation/PulseCountO1Engine.h"

#include <cstdint>

#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"
#include "tbb/tick_count.h"

#include "react/common/Types.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace react {
namespace pulse_count_o1_impl {

////////////////////////////////////////////////////////////////////////////////////////
/// PulseCountO1Node
////////////////////////////////////////////////////////////////////////////////////////
PulseCountO1Node::PulseCountO1Node() :
	Counter(0),
	ShouldUpdate(false),
	Pulsed(false),
	State(EState::init),
	marker_(0),
	weight_(1),
	cost_(1)
{
}

////////////////////////////////////////////////////////////////////////////////////////
/// PulseCountO1Turn
////////////////////////////////////////////////////////////////////////////////////////
PulseCountO1Turn::PulseCountO1Turn(TransactionData<PulseCountO1Turn>& transactionData) :
	TurnBase(transactionData),
	ExclusiveSequentialTransaction(transactionData.Input()),
	Marker(0)
{
}

////////////////////////////////////////////////////////////////////////////////////////
/// PulseCountO1Engine
////////////////////////////////////////////////////////////////////////////////////////
PulseCountO1Engine::PulseCountO1Engine() :
	curMarker_(1)
{
}

void PulseCountO1Engine::OnNodeAttach(PulseCountO1Node& node, PulseCountO1Node& parent)
{
	parent.Successors.Add(node);
	node.Predecessors.Add(parent);

	if (node.State != PulseCountO1Node::EState::attaching)
	{
		node.State = PulseCountO1Node::EState::attaching;
		node.SetMarker(nextMarker());
	}

	updateNodeWeight(parent, node.GetMarker(), node.Weight(), node.Cost());
	//tasks_.wait();
}

void PulseCountO1Engine::OnNodeDetach(PulseCountO1Node& node, PulseCountO1Node& parent)
{
	parent.Successors.Remove(node);
	node.Predecessors.Remove(parent);

	if (node.State != PulseCountO1Node::EState::detaching)
	{
		node.State = PulseCountO1Node::EState::detaching;
		node.SetMarker(nextMarker());
	}

	updateNodeWeight(parent, node.GetMarker(), -node.Weight(), -node.Cost());
}

void PulseCountO1Engine::OnTransactionCommit(TransactionData<PulseCountO1Turn>& transaction)
{
	PulseCountO1Turn turn(transaction);

	if (! BeginTransaction(turn))
		return;

	turn.Marker = nextMarker();

	//auto t0 = tbb::tick_count::now();
	//auto t1 = tbb::tick_count::now();
	//double d = (t1 - t0).seconds();
	//printf("Time %f\n");


	//auto t0 = tbb::tick_count::now();
	transaction.Input().RunAdmission(turn);
	runInitReachableNodesTask(std::move(changedInputNodes_), turn.Marker);
	tasks_.wait();
	//auto t1 = tbb::tick_count::now();
	//double d = (t1 - t0).seconds();
	//printf("Time %f\n", d);
	
	transaction.Input().RunPropagation(turn);
	tasks_.wait();

	changedInputNodes_.clear();

	EndTransaction(turn);
}

void PulseCountO1Engine::OnInputNodeAdmission(PulseCountO1Node& node, PulseCountO1Turn& turn)
{
	changedInputNodes_.push_back(&node);
}

void PulseCountO1Engine::OnNodePulse(PulseCountO1Node& node, PulseCountO1Turn& turn)
{
	nudgeChildren(node, true, turn);
}

void PulseCountO1Engine::OnNodeIdlePulse(PulseCountO1Node& node, PulseCountO1Turn& turn)
{
	nudgeChildren(node, false, turn);
}

void PulseCountO1Engine::OnNodeShift(PulseCountO1Node& node, PulseCountO1Node& oldParent, PulseCountO1Node& newParent, PulseCountO1Turn& turn)
{
	bool shouldTick = false;

	// oldParent.ShiftMutex (write)
	{
		NodeShiftMutexT::scoped_lock	lock(oldParent.ShiftMutex, true);

		oldParent.Successors.Remove(node);
		node.Predecessors.Remove(oldParent);
	}
	// ~oldParent.ShiftMutex (write)

	// newParent.ShiftMutex (write)
	{
		NodeShiftMutexT::scoped_lock	lock(newParent.ShiftMutex, true);
		
		newParent.Successors.Add(node);
		node.Predecessors.Add(newParent);

		if (! newParent.CheckMarker(turn.Marker))
		{
			shouldTick = true;
		}
		else
		{
			node.Counter = 1;
			node.ShouldUpdate = true;
		}
	}
	// ~newParent.ShiftMutex (write)

	if (shouldTick)
		node.Tick(&turn);
}

void PulseCountO1Engine::runInitReachableNodesTask(NodeVectorT leftNodes, MarkerT mark)
{
	NodeVectorT rightNodes;

	// Manage two balanced stacks of nodes.
	// Left size is always >= right size.
	// If left size exceeds threshold, move work to another task

	do
	{
		PulseCountO1Node* node = nullptr;

		if (leftNodes.size() > rightNodes.size())
		{
			node = leftNodes.back();
			leftNodes.pop_back();
		}
		else if (rightNodes.size() > 0)
		{
			node = rightNodes.back();
			rightNodes.pop_back();
		}
		else
		{
			break;
		}

		for (auto* succ : node->Successors)
		{
			succ->Counter++;

			if (!succ->SetMarker(mark))
				continue;
				
			(leftNodes.size() > rightNodes.size() ? rightNodes : leftNodes).push_back(succ);

			if (leftNodes.size() > 4)
			{
				tasks_.run(std::bind(&PulseCountO1Engine::runInitReachableNodesTask, this, std::move(leftNodes), mark));
				leftNodes.clear();
			}
		}
	}
	while (true);
}

void PulseCountO1Engine::processChild(PulseCountO1Node& node, bool update, PulseCountO1Turn& turn)
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

void PulseCountO1Engine::nudgeChildren(PulseCountO1Node& node, bool update, PulseCountO1Turn& turn)
{
	PulseCountO1Node* next = nullptr;

	// node.ShiftMutex (read)
	{
		NodeShiftMutexT::scoped_lock	lock(node.ShiftMutex);

		// Select first child as next node, dispatch tasks for rest
		for (auto* succ : node.Successors)
		{
			if (update)
				succ->ShouldUpdate = true;

			// Delay tick?
			if (succ->Counter-- > 1)
				continue;

			if (next != nullptr)
				tasks_.run(std::bind(&PulseCountO1Engine::processChild, this, std::ref(*succ), update, std::ref(turn)));
			else
				next = succ;
		}
	}
	// ~node.ShiftMutex (read)

	if (next)
		processChild(*next, update, turn);
}

void PulseCountO1Engine::updateNodeWeight(PulseCountO1Node& node, MarkerT mark, int weightDelta, int costDelta)
{
	node.AdjustWeight(weightDelta, costDelta);

	for (auto* pred : node.Predecessors)
	{
		if (pred->SetMarker(mark))
			updateNodeWeight(*pred, mark, weightDelta, costDelta);
	}
}

MarkerT PulseCountO1Engine::nextMarker()
{
	return curMarker_++;
}

} // ~namespace react::pulse_count_o1_impl
} // ~namespace react