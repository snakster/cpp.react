#include "react/propagation/PulseCountEngine.h"

#include <cstdint>

#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"
#include "tbb/tick_count.h"

#include "react/common/Types.h"

////////////////////////////////////////////////////////////////////////////////////////
REACT_IMPL_BEGIN
namespace pulsecount {

////////////////////////////////////////////////////////////////////////////////////////
/// Turn
////////////////////////////////////////////////////////////////////////////////////////
Turn::Turn(TurnIdT id, TurnFlagsT flags) :
	TurnBase(id, flags)
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
template <typename TTurn>
void EngineBase<TTurn>::OnNodeAttach(Node& node, Node& parent)
{
	parent.Successors.Add(node);
}

template <typename TTurn>
void EngineBase<TTurn>::OnNodeDetach(Node& node, Node& parent)
{
	parent.Successors.Remove(node);
}

template <typename TTurn>
void EngineBase<TTurn>::OnTurnInputChange(Node& node, TTurn& turn)
{
	changedInputs_.push_back(&node);
}

template <typename TTurn>
void EngineBase<TTurn>::OnTurnPropagate(TTurn& turn)
{
	for (auto* node : changedInputs_)
		tasks_.run(std::bind(&EngineBase<TTurn>::initTurn, this, std::ref(*node), std::ref(turn)));
	tasks_.wait();

	for (auto* node : changedInputs_)
		nudgeChildren(*node, true, turn);
	tasks_.wait();

	changedInputs_.clear();
}

template <typename TTurn>
void EngineBase<TTurn>::OnNodePulse(Node& node, TTurn& turn)
{
	nudgeChildren(node, true, turn);
}

template <typename TTurn>
void EngineBase<TTurn>::OnNodeIdlePulse(Node& node, TTurn& turn)
{
	nudgeChildren(node, false, turn);
}

template <typename TTurn>
void EngineBase<TTurn>::OnNodeShift(Node& node, Node& oldParent, Node& newParent, TTurn& turn)
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

template <typename TTurn>
void EngineBase<TTurn>::initTurn(Node& node, TTurn& turn)
{
	for (auto* succ : node.Successors)
	{
		succ->TickThreshold.fetch_add(1, std::memory_order_relaxed);

		if (!succ->Marked.exchange(true))
			tasks_.run(std::bind(&EngineBase<TTurn>::initTurn, this, std::ref(*succ), std::ref(turn)));
	}
}

template <typename TTurn>
void EngineBase<TTurn>::processChild(Node& node, bool update, TTurn& turn)
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

template <typename TTurn>
void EngineBase<TTurn>::nudgeChildren(Node& node, bool update, TTurn& turn)
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

			tasks_.run(std::bind(&EngineBase<TTurn>::processChild, this, std::ref(*succ), update, std::ref(turn)));
		}

		node.Marked = false;
	}
	// ~node.ShiftMutex (read)
}

// Explicit instantiation
template class EngineBase<Turn>;
template class EngineBase<DefaultQueueableTurn<Turn>>;

} // ~namespace pulsecount
REACT_IMPL_END