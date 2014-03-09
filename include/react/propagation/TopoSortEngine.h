#pragma once

#include <atomic>
#include <set>
#include <vector>

#include "tbb/concurrent_vector.h"
#include "tbb/concurrent_priority_queue.h"
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
/// SeqNode
////////////////////////////////////////////////////////////////////////////////////////
class SeqNode : public IReactiveNode
{
public:
	int		Level = 0;
	int		NewLevel = 0;
	bool	Queued = false;

	NodeVector<SeqNode>	Successors;
};

////////////////////////////////////////////////////////////////////////////////////////
/// ParNode
////////////////////////////////////////////////////////////////////////////////////////
class ParNode : public IReactiveNode
{
public:
	int				Level = 0;
	int				NewLevel = 0;
	atomic<bool>	Collected = false;	

	NodeVector<ParNode>	Successors;
};

////////////////////////////////////////////////////////////////////////////////////////
/// ShiftRequestData
////////////////////////////////////////////////////////////////////////////////////////
struct ShiftRequestData
{
	ParNode*	ShiftingNode;
	ParNode*	OldParent;
	ParNode*	NewParent;
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
/// EngineBase
////////////////////////////////////////////////////////////////////////////////////////
template <typename TNode, typename TTurn>
class EngineBase : public IReactiveEngine<TNode,TTurn>
{
public:
	using TopoQueueT = TopoQueue<TNode>;

	void OnNodeAttach(TNode& node, TNode& parent);
	void OnNodeDetach(TNode& node, TNode& parent);

	void OnTurnInputChange(TNode& node, TTurn& turn);
	void OnNodePulse(TNode& node, TTurn& turn);

protected:
	void invalidateSuccessors(TNode& node);

	virtual void processChildren(TNode& node, TTurn& turn) = 0;

	TopoQueueT	scheduledNodes_;
	task_group	tasks_;
};

////////////////////////////////////////////////////////////////////////////////////////
/// SeqEngineBase
////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurn>
class SeqEngineBase : public EngineBase<SeqNode,TTurn>
{
public:
	void OnTurnPropagate(TTurn& turn);
	void OnNodeShift(SeqNode& node, SeqNode& oldParent, SeqNode& newParent, TTurn& turn);

private:
	virtual void processChildren(SeqNode& node, TTurn& turn) override;
};

////////////////////////////////////////////////////////////////////////////////////////
/// ParEngineBase
////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurn>
class ParEngineBase : public EngineBase<ParNode,TTurn>
{
public:
	using ConcNodeVectorT = concurrent_vector<ParNode*>;
	using ShiftRequestVectorT = concurrent_vector<ShiftRequestData>;

	void OnTurnPropagate(TTurn& turn);
	void OnNodeShift(ParNode& node, ParNode& oldParent, ParNode& newParent, TTurn& turn);

private:
	void applyShift(ParNode& node, ParNode& oldParent, ParNode& newParent, TTurn& turn);

	virtual void processChildren(ParNode& node, TTurn& turn) override;

	ConcNodeVectorT		collectBuffer_;
	ShiftRequestVectorT	shiftRequests_;
};



////////////////////////////////////////////////////////////////////////////////////////
/// Concrete engines
////////////////////////////////////////////////////////////////////////////////////////
class BasicSeqEngine : public SeqEngineBase<Turn> {};
class QueuingSeqEngine : public DefaultQueuingEngine<SeqEngineBase,Turn> {};

class BasicParEngine :	public ParEngineBase<Turn> {};
class QueuingParEngine : public DefaultQueuingEngine<ParEngineBase,Turn> {};

} // ~namespace toposort
REACT_IMPL_END

////////////////////////////////////////////////////////////////////////////////////////
REACT_BEGIN

struct sequential;
struct sequential_queuing;
struct parallel;
struct parallel_queuing;

template <typename TMode>
class TopoSortEngine;

template <> class TopoSortEngine<sequential> : public REACT_IMPL::toposort::BasicSeqEngine {};
template <> class TopoSortEngine<sequential_queuing> : public REACT_IMPL::toposort::QueuingSeqEngine {};
template <> class TopoSortEngine<parallel> : public REACT_IMPL::toposort::BasicParEngine {};
template <> class TopoSortEngine<parallel_queuing> : public REACT_IMPL::toposort::QueuingParEngine {};

REACT_END