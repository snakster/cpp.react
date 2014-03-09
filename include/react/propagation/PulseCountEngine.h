#pragma once

#include <atomic>

#include "tbb/task_group.h"
#include "tbb/spin_rw_mutex.h"

#include "EngineBase.h"
#include "react/ReactiveDomain.h"
#include "react/common/Containers.h"
#include "react/common/Types.h"

////////////////////////////////////////////////////////////////////////////////////////
REACT_IMPL_BEGIN
namespace pulsecount {

using std::atomic;
using std::set;
using tbb::task_group;
using tbb::spin_rw_mutex;

////////////////////////////////////////////////////////////////////////////////////////
/// Turn
////////////////////////////////////////////////////////////////////////////////////////
struct Turn :
	public TurnBase,
	public TurnQueueManager::QueueEntry
{
public:
	Turn(TurnIdT id, TurnFlagsT flags);
};

////////////////////////////////////////////////////////////////////////////////////////
/// Node
////////////////////////////////////////////////////////////////////////////////////////
class Node : public IReactiveNode
{
public:
	typedef spin_rw_mutex	ShiftMutexT;

	Node();

	ShiftMutexT			ShiftMutex;
	NodeVector<Node>	Successors;

	atomic<int>		TickThreshold;
	atomic<bool>	ShouldUpdate;
	atomic<bool>	Marked;
};

////////////////////////////////////////////////////////////////////////////////////////
/// PulseCountEngine
////////////////////////////////////////////////////////////////////////////////////////
class PulseCountEngine :
	public IReactiveEngine<Node,Turn>,
	public TurnQueueManager
{
public:
	typedef Node::ShiftMutexT	NodeShiftMutexT;	

	void OnNodeAttach(Node& node, Node& parent);
	void OnNodeDetach(Node& node, Node& parent);

	void OnTurnAdmissionStart(Turn& turn);
	void OnTurnAdmissionEnd(Turn& turn);
	void OnTurnEnd(Turn& turn);

	void OnTurnInputChange(Node& node, Turn& turn);
	void OnTurnPropagate(Turn& turn);

	void OnNodePulse(Node& node, Turn& turn);
	void OnNodeIdlePulse(Node& node, Turn& turn);

	void OnNodeShift(Node& node, Node& oldParent, Node& newParent, Turn& turn);

private:
	void initTurn(Node& node, Turn& turn);

	void processChild(Node& node, bool update, Turn& turn);
	void nudgeChildren(Node& parent, bool update, Turn& turn);

	std::vector<Node*>	changedInputs_;
	task_group			tasks_;
};

} // ~namespace pulsecount
REACT_IMPL_END

REACT_BEGIN

using REACT_IMPL::pulsecount::PulseCountEngine;

REACT_END