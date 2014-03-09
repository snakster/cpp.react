#pragma once

#include <atomic>
#include <vector>

#include "tbb/spin_rw_mutex.h"
#include "tbb/task_group.h"

#include "EngineBase.h"
#include "react/ReactiveDomain.h"
#include "react/common/Containers.h"
#include "react/common/Types.h"

////////////////////////////////////////////////////////////////////////////////////////
REACT_IMPL_BEGIN
namespace pulsecount_o1 {

using std::atomic;
using std::vector;
using tbb::task_group;
using tbb::spin_rw_mutex;

typedef int		MarkerT;

////////////////////////////////////////////////////////////////////////////////////////
/// Node
////////////////////////////////////////////////////////////////////////////////////////
class Node : public IReactiveNode
{
public:
	enum class EState { init, attaching, detaching };

	typedef spin_rw_mutex		ShiftMutexT;

	Node();

	void AdjustWeight(int weightDelta, int costDelta)
	{
		weight_ += weightDelta;
		cost_ += costDelta;
	}

	bool	SetMarker(MarkerT marker)			{ return marker_.exchange(marker, std::memory_order_relaxed) != marker; }
	MarkerT	GetMarker() const					{ return marker_.load(std::memory_order_relaxed); }
	void	ClearMarker()						{ marker_.store(0, std::memory_order_relaxed); }

	int		Weight() const	{ return weight_; }
	int		Cost() const	{ return cost_; }

	NodeVector<Node>	Successors;
	NodeVector<Node>	Predecessors;

	atomic<int>		Counter;
	atomic<bool>	ShouldUpdate;

	ShiftMutexT	ShiftMutex;

	EState	State;

private:
	atomic<MarkerT>	marker_;

	int		weight_;
	int		cost_;
};

////////////////////////////////////////////////////////////////////////////////////////
/// Turn
////////////////////////////////////////////////////////////////////////////////////////
class Turn : public TurnBase
{
public:
	Turn(TurnIdT id, TurnFlagsT flags);

	MarkerT	Marker;
};

////////////////////////////////////////////////////////////////////////////////////////
/// EngineBase
////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurn>
class EngineBase : public IReactiveEngine<Node,TTurn>
{
public:
	void OnNodeAttach(Node& node, Node& parent);
	void OnNodeDetach(Node& node, Node& parent);

	void OnTurnInputChange(Node& node, TTurn& turn);
	void OnTurnPropagate(TTurn& turn);

	void OnNodePulse(Node& node, TTurn& turn);
	void OnNodeIdlePulse(Node& node, TTurn& turn);

	void OnNodeShift(Node& node, Node& oldParent, Node& newParent, TTurn& turn);

private:
	typedef Node::ShiftMutexT	NodeShiftMutexT;
	typedef vector<Node*>		NodeVectorT;

	void	runInitReachableNodesTask(NodeVectorT leftNodes, MarkerT marker);

	void	updateNodeWeight(Node& node, MarkerT marker, int weightDelta, int costDelta);

	void	processChild(Node& node, bool update, TTurn& turn);
	void	nudgeChildren(Node& parent, bool update, TTurn& turn);

	MarkerT	nextMarker();

	task_group		tasks_;
	MarkerT			curMarker_ = 1;
	
	NodeVectorT		changedInputs_;
};

class BasicEngine :	public EngineBase<Turn> {};
class QueuingEngine : public DefaultQueuingEngine<EngineBase,Turn> {};

} // ~namespace pulsecount_o1
REACT_IMPL_END

REACT_BEGIN

struct parallel;
struct parallel_queuing;

template <typename TMode = parallel_queuing>
class PulseCountO1Engine;

template <> class PulseCountO1Engine<parallel> : public REACT_IMPL::pulsecount_o1::BasicEngine {};
template <> class PulseCountO1Engine<parallel_queuing> : public REACT_IMPL::pulsecount_o1::QueuingEngine {};

REACT_END