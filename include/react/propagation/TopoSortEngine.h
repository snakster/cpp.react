#pragma once

#include <atomic>
#include <set>
#include <vector>

#include "tbb/concurrent_vector.h"
#include "tbb/task_group.h"
#include "tbb/queuing_rw_mutex.h"

#include "EngineBase.h"
#include "react/ReactiveDomain.h"
#include "react/common/Containers.h"
#include "react/common/TopoQueue.h"
#include "react/common/Types.h"

////////////////////////////////////////////////////////////////////////////////////////
REACT_IMPL_BEGIN
namespace toposort {

using std::atomic;
using std::set;
using std::vector;
using tbb::concurrent_vector;
using tbb::task_group;

////////////////////////////////////////////////////////////////////////////////////////
/// Node
////////////////////////////////////////////////////////////////////////////////////////
class Node : public IReactiveNode
{
public:
	int				Level;
	int				NewLevel;
	atomic<bool>	Collected;

	Node();

	NodeVector<Node>	Successors;
};

////////////////////////////////////////////////////////////////////////////////////////
/// ShiftRequestData
////////////////////////////////////////////////////////////////////////////////////////
struct ShiftRequestData
{
	Node*	ShiftingNode;
	Node*	OldParent;
	Node*	NewParent;
};

////////////////////////////////////////////////////////////////////////////////////////
/// Turn
////////////////////////////////////////////////////////////////////////////////////////
class Turn :
	public TurnBase,
	public TurnQueueManager::QueueEntry
{
public:
	Turn(TurnIdT id, TurnFlagsT flags);
};

////////////////////////////////////////////////////////////////////////////////////////
/// TopoSortEngine
////////////////////////////////////////////////////////////////////////////////////////
class TopoSortEngine :
	public IReactiveEngine<Node,Turn>,
	public TurnQueueManager
{
public:
	using NodeSetT = set<Node*>;
	using ConcNodeVectorT = concurrent_vector<Node*>;
	using ShiftRequestVectorT = concurrent_vector<ShiftRequestData>;
	using TopoQueueT = TopoQueue<Node>;

	TopoSortEngine();

	void OnNodeAttach(Node& node, Node& parent);
	void OnNodeDetach(Node& node, Node& parent);

	void OnTurnAdmissionStart(Turn& turn);
	void OnTurnAdmissionEnd(Turn& turn);
	void OnTurnEnd(Turn& turn);

	void OnTurnInputChange(Node& node, Turn& turn);
	void OnTurnPropagate(Turn& turn);

	void OnNodePulse(Node& node, Turn& turn);
	void OnNodeShift(Node& node, Node& oldParent, Node& newParent, Turn& turn);

private:
	void applyShift(Node& node, Node& oldParent, Node& newParent, Turn& turn);

	void processChildren(Node& node, Turn& turn);
	void invalidateSuccessors(Node& node);

	TopoQueueT			scheduledNodes_;
	ConcNodeVectorT		collectBuffer_;
	ShiftRequestVectorT	shiftRequests_;

	task_group	tasks_;
};

} // ~namespace toposort
REACT_IMPL_END

REACT_BEGIN

using REACT_IMPL::toposort::TopoSortEngine;

REACT_END