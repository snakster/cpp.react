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
using std::vector;
using tbb::queuing_mutex;
using tbb::task_group;
using tbb::spin_mutex;

////////////////////////////////////////////////////////////////////////////////////////
/// Turn
////////////////////////////////////////////////////////////////////////////////////////
class Turn :
	public TurnBase,
	public ExclusiveTurnManager::ExclusiveTurn
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
	using ShiftMutexT = spin_mutex;

	Node();

	bool	MarkForSchedule();
	bool	Evaluate(Turn& turn);

	NodeVector<Node>	Successors;
	
	ShiftMutexT		ShiftMutex;

private:
	using EvalMutexT = spin_mutex;

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
	public ExclusiveTurnManager
{
public:
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
	using OutputMutexT = queuing_mutex;
	using NodeShiftMutexT = Node::ShiftMutexT;

	set<Node*>		outputNodes_;
	OutputMutexT	outputMutex_;
	vector<Node*>	changedInputs_;

	task_group		tasks_;

	void pulse(Node& node, Turn& turn);
	void process(Node& node, Turn& turn);
};

} // ~namespace react::flooding_impl

using flooding_impl::FloodingEngine;

} // ~namespace react