#pragma once

#include <atomic>
#include <set>

#include "tbb/concurrent_vector.h"
#include "tbb/task_group.h"
#include "tbb/queuing_rw_mutex.h"

#include "EngineBase.h"
#include "react/ReactiveDomain.h"
#include "react/common/Containers.h"
#include "react/common/TopoQueue.h"
#include "react/common/Types.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace react {
namespace topo_sort_impl {

using std::atomic;
using std::set;
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
	public TurnBase<Turn>,
	public ExclusiveSequentialTransaction<Turn>
{
public:
	explicit Turn(TransactionData<Turn>& transactionData);
};

////////////////////////////////////////////////////////////////////////////////////////
/// TopoSortEngine
////////////////////////////////////////////////////////////////////////////////////////
class TopoSortEngine :
	public IReactiveEngine<Node,Turn>,
	public ExclusiveSequentialTransactionEngine<Turn>
{
public:
	typedef set<Node*>	NodeSet;

	typedef concurrent_vector<Node*>		ConcNodeVector;
	typedef concurrent_vector<ShiftRequestData>		ShiftRequestVector;
	typedef TopoQueue<Node>					TopoQueue;

	TopoSortEngine();

	void OnNodeAttach(Node& node, Node& parent);
	void OnNodeDetach(Node& node, Node& parent);

	void OnTransactionCommit(TransactionData<Turn>& transaction);

	void OnNodePulse(Node& node, Turn& turn);
	void OnNodeShift(Node& node, Node& oldParent, Node& newParent, Turn& turn);

private:
	void applyShift(Node& node, Node& oldParent, Node& newParent, Turn& turn);

	void processChildren(Node& node, Turn& turn);
	void invalidateSuccessors(Node& node);

	TopoQueue			scheduledNodes_;
	ConcNodeVector		collectBuffer_;
	ShiftRequestVector	shiftRequests_;

	task_group	tasks_;
};

} // ~namespace react::topo_sort_impl

using topo_sort_impl::TopoSortEngine;

} // ~namespace react
