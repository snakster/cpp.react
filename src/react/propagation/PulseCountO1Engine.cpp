#include "react/propagation/PulseCountO1Engine.h"

#include <cstdint>

#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"
#include "tbb/tick_count.h"

#include "react/common/Types.h"

////////////////////////////////////////////////////////////////////////////////////////
REACT_IMPL_BEGIN
namespace pulsecount_o1 {

////////////////////////////////////////////////////////////////////////////////////////
/// Node
////////////////////////////////////////////////////////////////////////////////////////
Node::Node() :
	Counter{ 0 },
	ShouldUpdate{ false },
	State{ EState::init },
	marker_{ 0 },
	weight_{ 1 },
	cost_{ 1 }
{
}

////////////////////////////////////////////////////////////////////////////////////////
/// Turn
////////////////////////////////////////////////////////////////////////////////////////
Turn::Turn(TurnIdT id, TurnFlagsT flags) :
	TurnBase(id, flags),
	ExclusiveTurnManager::ExclusiveTurn(flags),
	Marker{ 0 }
{
}

////////////////////////////////////////////////////////////////////////////////////////
/// PulseCountO1Engine
////////////////////////////////////////////////////////////////////////////////////////
PulseCountO1Engine::PulseCountO1Engine() :
	curMarker_{ 1 }
{
}

void PulseCountO1Engine::OnNodeAttach(Node& node, Node& parent)
{
	parent.Successors.Add(node);
	node.Predecessors.Add(parent);

	if (node.State != Node::EState::attaching)
	{
		node.State = Node::EState::attaching;
		node.SetMarker(nextMarker());
	}

	updateNodeWeight(parent, node.GetMarker(), node.Weight(), node.Cost());
	//tasks_.wait();
}

void PulseCountO1Engine::OnNodeDetach(Node& node, Node& parent)
{
	parent.Successors.Remove(node);
	node.Predecessors.Remove(parent);

	if (node.State != Node::EState::detaching)
	{
		node.State = Node::EState::detaching;
		node.SetMarker(nextMarker());
	}

	updateNodeWeight(parent, node.GetMarker(), -node.Weight(), -node.Cost());
}

void PulseCountO1Engine::OnTurnAdmissionStart(Turn& turn)
{
	StartTurn(turn);
}

void PulseCountO1Engine::OnTurnAdmissionEnd(Turn& turn)
{
	turn.RunMergedInputs();
}

void PulseCountO1Engine::OnTurnInputChange(Node& node, Turn& turn)
{
	changedInputs_.push_back(&node);
}

void PulseCountO1Engine::OnTurnPropagate(Turn& turn)
{
	turn.Marker = nextMarker();

	// Note: Copies the vector
	runInitReachableNodesTask(changedInputs_, turn.Marker);
	tasks_.wait();

	for (auto* node : changedInputs_)
		nudgeChildren(*node, true, turn);
	tasks_.wait();

	changedInputs_.clear();
}

void PulseCountO1Engine::OnTurnEnd(Turn& turn)
{
	EndTurn(turn);
}

void PulseCountO1Engine::OnNodePulse(Node& node, Turn& turn)
{
	nudgeChildren(node, true, turn);
}

void PulseCountO1Engine::OnNodeIdlePulse(Node& node, Turn& turn)
{
	nudgeChildren(node, false, turn);
}

void PulseCountO1Engine::OnNodeShift(Node& node, Node& oldParent, Node& newParent, Turn& turn)
{
	bool shouldTick = false;

	{// oldParent.ShiftMutex (write)
		NodeShiftMutexT::scoped_lock	lock(oldParent.ShiftMutex, true);

		oldParent.Successors.Remove(node);
		node.Predecessors.Remove(oldParent);
	}// ~oldParent.ShiftMutex (write)

	
	{// newParent.ShiftMutex (write)
		NodeShiftMutexT::scoped_lock	lock(newParent.ShiftMutex, true);
		
		newParent.Successors.Add(node);
		node.Predecessors.Add(newParent);

		if (! newParent.GetMarker() == turn.Marker)
		{
			shouldTick = true;
		}
		else
		{
			node.Counter = 1;
			node.ShouldUpdate = true;
		}
	}// ~newParent.ShiftMutex (write)

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
		Node* node = nullptr;

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

void PulseCountO1Engine::processChild(Node& node, bool update, Turn& turn)
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

void PulseCountO1Engine::nudgeChildren(Node& node, bool update, Turn& turn)
{
	Node* next = nullptr;

	{// node.ShiftMutex (read)
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

		node.ClearMarker();
	}// ~node.ShiftMutex (read)

	if (next)
		processChild(*next, update, turn);
}

void PulseCountO1Engine::updateNodeWeight(Node& node, MarkerT mark, int weightDelta, int costDelta)
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

} // ~namespace pulsecount_o1
REACT_IMPL_END