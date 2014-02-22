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
namespace react {
namespace pulse_count_o1_impl {

using std::atomic;
using std::vector;
using tbb::task_group;
using tbb::spin_rw_mutex;

typedef int		MarkerT;

////////////////////////////////////////////////////////////////////////////////////////
/// PulseCountO1Node
////////////////////////////////////////////////////////////////////////////////////////
class PulseCountO1Node : public IReactiveNode
{
public:
	enum class EState { init, attaching, detaching };

	typedef spin_rw_mutex		ShiftMutexT;

	PulseCountO1Node();

	void AdjustWeight(int weightDelta, int costDelta)
	{
		weight_ += weightDelta;
		cost_ += costDelta;
	}

	bool	SetMarker(MarkerT marker)			{ return marker_.exchange(marker) != marker; }
	bool	CheckMarker(MarkerT marker) const	{ return marker_ == marker; }
	MarkerT	GetMarker() const					{ return marker_; }

	int		Weight() const	{ return weight_; }
	int		Cost() const	{ return cost_; }

	NodeVector<PulseCountO1Node>	Successors;
	NodeVector<PulseCountO1Node>	Predecessors;

	atomic<int>		Counter;
	atomic<bool>	ShouldUpdate;
	atomic<bool>	Pulsed;

	ShiftMutexT	ShiftMutex;

	EState	State;

private:
	atomic<MarkerT>	marker_;

	int		weight_;
	int		cost_;
};

////////////////////////////////////////////////////////////////////////////////////////
/// PulseCountO1Turn
////////////////////////////////////////////////////////////////////////////////////////
struct PulseCountO1Turn :
	public TurnBase<PulseCountO1Turn>,
	public ExclusiveSequentialTransaction<PulseCountO1Turn>
{
public:
	explicit PulseCountO1Turn(TransactionData<PulseCountO1Turn>& transactionData);

	MarkerT	Marker;
};

////////////////////////////////////////////////////////////////////////////////////////
/// PulseCountO1Engine
////////////////////////////////////////////////////////////////////////////////////////
class PulseCountO1Engine :
	public IReactiveEngine<PulseCountO1Node,PulseCountO1Turn>,
	public ExclusiveSequentialTransactionEngine<PulseCountO1Turn>
{
public:
	PulseCountO1Engine();

	virtual void OnNodeAttach(PulseCountO1Node& node, PulseCountO1Node& parent);
	virtual void OnNodeDetach(PulseCountO1Node& node, PulseCountO1Node& parent);

	virtual void OnTransactionCommit(TransactionData<PulseCountO1Turn>& transaction);

	virtual void OnInputNodeAdmission(PulseCountO1Node& node, PulseCountO1Turn& turn);

	virtual void OnNodePulse(PulseCountO1Node& node, PulseCountO1Turn& turn);
	virtual void OnNodeIdlePulse(PulseCountO1Node& node, PulseCountO1Turn& turn);

	virtual void OnNodeShift(PulseCountO1Node& node, PulseCountO1Node& oldParent, PulseCountO1Node& newParent, PulseCountO1Turn& turn);

private:
	typedef PulseCountO1Node::ShiftMutexT	NodeShiftMutexT;
	typedef vector<PulseCountO1Node*>		NodeVectorT;

	void	runInitReachableNodesTask(NodeVectorT leftNodes, MarkerT marker);

	void	updateNodeWeight(PulseCountO1Node& node, MarkerT marker, int weightDelta, int costDelta);

	void	processChild(PulseCountO1Node& node, bool update, PulseCountO1Turn& turn);
	void	nudgeChildren(PulseCountO1Node& parent, bool update, PulseCountO1Turn& turn);

	MarkerT	nextMarker();

	task_group		tasks_;
	MarkerT			curMarker_;
	NodeVectorT		changedInputNodes_;
};

} // ~namespace react::pulse_count_o1_impl

using pulse_count_o1_impl::PulseCountO1Engine;

} // ~namespace react