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
struct Turn : public TurnBase
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
/// EngineBase
////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurn>
class EngineBase : public IReactiveEngine<Node,TTurn>
{
public:
	typedef Node::ShiftMutexT	NodeShiftMutexT;	

	void OnNodeAttach(Node& node, Node& parent);
	void OnNodeDetach(Node& node, Node& parent);

	void OnTurnInputChange(Node& node, TTurn& turn);
	void OnTurnPropagate(TTurn& turn);

	void OnNodePulse(Node& node, TTurn& turn);
	void OnNodeIdlePulse(Node& node, TTurn& turn);

	void OnNodeShift(Node& node, Node& oldParent, Node& newParent, TTurn& turn);

private:
	void initTurn(Node& node, TTurn& turn);

	void processChild(Node& node, bool update, TTurn& turn);
	void nudgeChildren(Node& parent, bool update, TTurn& turn);

	std::vector<Node*>	changedInputs_;
	task_group			tasks_;
};

class BasicEngine :	public EngineBase<Turn> {};
class QueuingEngine : public DefaultQueuingEngine<EngineBase,Turn> {};

} // ~namespace pulsecount
REACT_IMPL_END

REACT_BEGIN

struct parallel;
struct parallel_queuing;

template <typename TMode = parallel_queuing>
class PulseCountEngine;

template <> class PulseCountEngine<parallel> : public REACT_IMPL::pulsecount::BasicEngine {};
template <> class PulseCountEngine<parallel_queuing> : public REACT_IMPL::pulsecount::QueuingEngine {};

REACT_END