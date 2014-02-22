#pragma once

#include <vector>

#include "tbb/queuing_mutex.h"
#include "tbb/spin_mutex.h"
#include "tbb/task_group.h"

#include "EngineBase.h"
#include "react/ReactiveDomain.h"
#include "react/common/Containers.h"
#include "react/common/Util.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace react {
namespace flooding_impl {

using std::atomic;
using std::set;
using tbb::queuing_mutex;
using tbb::task_group;
using tbb::spin_mutex;

////////////////////////////////////////////////////////////////////////////////////////
/// Turn
////////////////////////////////////////////////////////////////////////////////////////
struct Turn :
	public TurnBase<Turn>,
	public ExclusiveSequentialTransaction<Turn>
{
public:
	explicit Turn(TransactionData<Turn>& transactionData);
};


////////////////////////////////////////////////////////////////////////////////////////
/// Node
////////////////////////////////////////////////////////////////////////////////////////
class Node : public IReactiveNode
{
public:
	typedef spin_mutex		ShiftMutexT;

	Node();

	bool	MarkForSchedule();
	bool	Evaluate(Turn& turn);

	NodeVector<Node>	Successors;
	
	ShiftMutexT		ShiftMutex;

private:
	typedef spin_mutex		EvalMutexT;

	atomic<bool>	isScheduled_;

	EvalMutexT		mutex_;

	bool	shouldReprocess_;
	bool	isProcessing_;
};

////////////////////////////////////////////////////////////////////////////////////////
/// FloodingEngine - A simple recursive engine.
////////////////////////////////////////////////////////////////////////////////////////
class FloodingEngine :
	public IReactiveEngine<Node,Turn>,
	public ExclusiveSequentialTransactionEngine<Turn>
{
public:
	void OnTransactionCommit(TransactionData<Turn>& transaction);

	void OnNodeAttach(Node& node, Node& parent);
	void OnNodeDetach(Node& node, Node& parent);

	void OnNodePulse(Node& node, Turn& turn);
	void OnNodeShift(Node& node, Node& oldParent, Node& newParent, Turn& turn);

private:
	typedef set<Node*>		NodeSetT;
	typedef queuing_mutex			OutputMutexT;
	typedef Node::ShiftMutexT	NodeShiftMutexT;

	NodeSetT		outputNodes_;
	OutputMutexT	outputMutex_;

	task_group		tasks_;

	void process(Node& node, Turn& turn);
};

} // ~namespace react::flooding_impl

using flooding_impl::FloodingEngine;

} // ~namespace react