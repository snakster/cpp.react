#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <set>
#include <vector>

#include "tbb/concurrent_vector.h"
#include "tbb/queuing_rw_mutex.h"
#include "tbb/task_group.h"

#include "EngineBase.h"
#include "react/ReactiveDomain.h"
#include "react/common/Containers.h"
#include "react/common/TopoQueue.h"
#include "react/common/Types.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace react {
namespace topo_sort_o1_impl {

using std::atomic;
using std::condition_variable;
using std::mutex;
using std::pair;
using std::set;
using std::vector;
using tbb::concurrent_vector;
using tbb::queuing_rw_mutex;
using tbb::task_group;

////////////////////////////////////////////////////////////////////////////////////////
/// Node
////////////////////////////////////////////////////////////////////////////////////////
class Node : public IReactiveNode
{
public:
	int				Level;
	atomic<bool>	Collected;
	atomic<bool>	Invalidated;

	Node();

	NodeVector<Node>	Successors;
};

////////////////////////////////////////////////////////////////////////////////////////
/// InvalidateData
////////////////////////////////////////////////////////////////////////////////////////
struct InvalidateData
{
	Node*	ShiftingNode;
	Node*	OldParent;
	Node*	NewParent;
};

////////////////////////////////////////////////////////////////////////////////////////
/// Turn
////////////////////////////////////////////////////////////////////////////////////////
class Turn : public TurnBase<Turn>
{
public:
	typedef concurrent_vector<Node*>			ConcNodeVector;
	typedef vector<Node*>						NodeVector;
	typedef set<pair<int,int>>					IntervalSet;
	typedef concurrent_vector<InvalidateData>	InvalidateDataVector;
	typedef vector<Turn*>						TurnVector;
	typedef TopoQueue<Node>						TopoQueue;

	explicit Turn(TransactionData<Turn>& transactionData);

	TopoQueue				ScheduledNodes;
	ConcNodeVector			CollectBuffer;
	InvalidateDataVector	InvalidateRequests;
	task_group				Tasks;

	int		CurrentLevel() const	{ return currentLevel_; }

	bool	AdvanceLevel();
	void	SetMaxLevel(int level);
	void	WaitForMaxLevel(int targetLevel);

	void	UpdateSuccessor();

	void	Append(Turn* turn);

	void	Remove();

	void	AdjustUpperBound(int level);

	bool	TryMerge(Turn& other);

private:
	TransactionInput<Turn>& input_;

	IntervalSet		levelIntervals_;

	Turn*	predecessor_;
	Turn*	successor_;

	int		currentLevel_;
	int		maxLevel_;			/// This turn may only advance up to maxLevel
	int		minLevel_;			/// successor.maxLevel = this.minLevel - 1

	int		curUpperBound_;

	mutex					advanceMutex_;
	condition_variable		advanceCondition_;

	TurnVector	mergedTurns_;
};

////////////////////////////////////////////////////////////////////////////////////////
/// TopoSortO1Engine
////////////////////////////////////////////////////////////////////////////////////////
class TopoSortO1Engine : public IReactiveEngine<Node,Turn>
{
public:
	typedef queuing_rw_mutex		SeqMutex;
	typedef set<Node*>	NodeSet;

	TopoSortO1Engine();

	virtual void OnNodeAttach(Node& node, Node& parent);
	virtual void OnNodeDetach(Node& node, Node& parent);

	virtual void OnTransactionCommit(TransactionData<Turn>& transaction);

	virtual void OnNodePulse(Node& node, Turn& turn);
	virtual void OnNodeShift(Node& node, Node& oldParent, Node& newParent, Turn& turn);

private:
	SeqMutex	sequenceMutex_;
	Turn*		lastTurn_;

	NodeSet		dynamicNodes_;
	int			maxDynamicLevel_;

	void applyInvalidate(Node& node, Node& oldParent, Node& newParent, Turn& turn);

	void processChildren(Node& node, Turn& turn);
	void recalculateLevels(Node& node);

	bool addTurn(Turn& turn, bool allowMerging);
	void removeTurn(Turn& turn);

	void advanceTurn(Turn& turn);
};

} // ~namespace react::topo_sort_o1_impl

using topo_sort_o1_impl::TopoSortO1Engine;

} // ~namespace react
