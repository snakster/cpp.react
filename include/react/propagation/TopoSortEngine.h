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
class Turn : public TurnBase
{
public:
	Turn(TurnIdT id, TurnFlagsT flags);
};

////////////////////////////////////////////////////////////////////////////////////////
/// TopoSortEngine
////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurn>
class EngineBase : public IReactiveEngine<Node,TTurn>
{
public:
	using NodeSetT = set<Node*>;
	using ConcNodeVectorT = concurrent_vector<Node*>;
	using ShiftRequestVectorT = concurrent_vector<ShiftRequestData>;
	using TopoQueueT = TopoQueue<Node>;

	void OnNodeAttach(Node& node, Node& parent);
	void OnNodeDetach(Node& node, Node& parent);

	void OnTurnInputChange(Node& node, TTurn& turn);
	void OnTurnPropagate(TTurn& turn);

	void OnNodePulse(Node& node, TTurn& turn);
	void OnNodeShift(Node& node, Node& oldParent, Node& newParent, TTurn& turn);

private:
	void applyShift(Node& node, Node& oldParent, Node& newParent, TTurn& turn);

	void processChildren(Node& node, TTurn& turn);
	void invalidateSuccessors(Node& node);

	TopoQueueT			scheduledNodes_;
	ConcNodeVectorT		collectBuffer_;
	ShiftRequestVectorT	shiftRequests_;

	task_group	tasks_;
};

class BasicEngine :	public EngineBase<Turn> {};
class QueuingEngine : public DefaultQueuingEngine<EngineBase,Turn> {};

} // ~namespace toposort
REACT_IMPL_END

////////////////////////////////////////////////////////////////////////////////////////
REACT_BEGIN

struct parallel;
struct parallel_queuing;

template <typename TMode = parallel_queuing>
class TopoSortEngine;

template <> class TopoSortEngine<parallel> : public REACT_IMPL::toposort::BasicEngine {};
template <> class TopoSortEngine<parallel_queuing> : public REACT_IMPL::toposort::QueuingEngine {};

REACT_END