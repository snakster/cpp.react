
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <set>
#include <utility>
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
using std::condition_variable;
using std::function;
using std::mutex;
using std::pair;
using std::set;
using std::vector;
using tbb::concurrent_vector;
using tbb::queuing_rw_mutex;
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
/// ExclusiveTurn
////////////////////////////////////////////////////////////////////////////////////////
class ExclusiveTurn : public TurnBase
{
public:
	ExclusiveTurn(TurnIdT id, TurnFlagsT flags);
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
class BasicSeqEngine : public SeqEngineBase<ExclusiveTurn> {};
class QueuingSeqEngine : public DefaultQueuingEngine<SeqEngineBase,ExclusiveTurn> {};

class BasicParEngine :	public ParEngineBase<ExclusiveTurn> {};
class QueuingParEngine : public DefaultQueuingEngine<ParEngineBase,ExclusiveTurn> {};

////////////////////////////////////////////////////////////////////////////////////////
/// PipeliningTurn
////////////////////////////////////////////////////////////////////////////////////////
class PipeliningTurn : public TurnBase
{
public:
	using ConcNodeVectorT = concurrent_vector<ParNode*>;
	using NodeVectT = vector<ParNode*>;
	using IntervalSetT = set<pair<int,int>>;
	using ShiftRequestVectorT = concurrent_vector<ShiftRequestData>;
	using TopoQueueT = TopoQueue<ParNode>;

	PipeliningTurn(TurnIdT id, TurnFlagsT flags);

	int		CurrentLevel() const	{ return currentLevel_; }

	bool	AdvanceLevel();
	void	SetMaxLevel(int level);
	void	WaitForMaxLevel(int targetLevel);

	void	UpdateSuccessor();

	void	Append(PipeliningTurn* turn);

	void	Remove();

	void	AdjustUpperBound(int level);

	template <typename F>
	bool TryMerge(F&& inputFunc, BlockingCondition& caller)
	{
		if (!isMergeable_)
			return false;

		bool merged = false;
		
		{// advMutex_
			std::lock_guard<std::mutex> scopedLock(advMutex_);

			// Only merge if target is still blocked
			if (currentLevel_ == -1)
			{
				merged = true;
				caller.Block();
				merged_.emplace_back(std::make_pair(std::forward<F>(inputFunc), &caller));
			}
		}// ~advMutex_

		return merged;
	}

	void RunMergedInputs() const;

	TopoQueueT			ScheduledNodes;
	ConcNodeVectorT		CollectBuffer;
	ShiftRequestVectorT	ShiftRequests;
	task_group			Tasks;

private:
	using MergedDataVectT = vector<pair<function<void()>,BlockingCondition*>>;

	const bool			isMergeable_;
	MergedDataVectT		merged_;

	IntervalSetT		levelIntervals_;

	PipeliningTurn*	predecessor_ = nullptr;
	PipeliningTurn*	successor_ = nullptr;

	int		currentLevel_ = -1;
	int		maxLevel_ = INT_MAX;			/// This turn may only advance up to maxLevel
	int		minLevel_ = -1;			/// successor.maxLevel = this.minLevel - 1

	int		curUpperBound_ = -1;

	mutex				advMutex_;
	condition_variable	advCondition_;
};

////////////////////////////////////////////////////////////////////////////////////////
/// PipeliningEngine
////////////////////////////////////////////////////////////////////////////////////////
class PipeliningEngine : public IReactiveEngine<ParNode,PipeliningTurn>
{
public:
	using SeqMutexT = queuing_rw_mutex;
	using NodeSetT = set<ParNode*>;

	void OnNodeAttach(ParNode& node, ParNode& parent);
	void OnNodeDetach(ParNode& node, ParNode& parent);

	void OnTurnAdmissionStart(PipeliningTurn& turn);
	void OnTurnAdmissionEnd(PipeliningTurn& turn);
	void OnTurnEnd(PipeliningTurn& turn);

	void OnTurnInputChange(ParNode& node, PipeliningTurn& turn);
	void OnNodePulse(ParNode& node, PipeliningTurn& turn);

	void OnTurnPropagate(PipeliningTurn& turn);

	void OnNodeShift(ParNode& node, ParNode& oldParent, ParNode& newParent, PipeliningTurn& turn);

	template <typename F>
	inline bool TryMerge(F&& inputFunc)
	{
		bool merged = false;

		BlockingCondition caller;

		{// seqMutex_
			SeqMutexT::scoped_lock lock(seqMutex_);

			if (tail_)
				merged = tail_->TryMerge(std::forward<F>(inputFunc), caller);
		}// ~seqMutex_

		if (merged)
			caller.WaitForUnblock();

		return merged;
	}

private:
	void applyShift(ParNode& node, ParNode& oldParent, ParNode& newParent, PipeliningTurn& turn);
	void processChildren(ParNode& node, PipeliningTurn& turn);
	void invalidateSuccessors(ParNode& node);

	void advanceTurn(PipeliningTurn& turn);

	SeqMutexT			seqMutex_;
	PipeliningTurn*		tail_ = nullptr;

	NodeSetT	dynamicNodes_;
	int			maxDynamicLevel_;
};

} // ~namespace toposort
REACT_IMPL_END

////////////////////////////////////////////////////////////////////////////////////////
REACT_BEGIN

struct sequential;
struct sequential_queuing;
struct parallel;
struct parallel_queuing;
struct parallel_pipelining;

template <typename TMode>
class TopoSortEngine;

template <> class TopoSortEngine<sequential> : public REACT_IMPL::toposort::BasicSeqEngine {};
template <> class TopoSortEngine<sequential_queuing> : public REACT_IMPL::toposort::QueuingSeqEngine {};
template <> class TopoSortEngine<parallel> : public REACT_IMPL::toposort::BasicParEngine {};
template <> class TopoSortEngine<parallel_queuing> : public REACT_IMPL::toposort::QueuingParEngine {};
template <> class TopoSortEngine<parallel_pipelining> : public REACT_IMPL::toposort::PipeliningEngine {};

REACT_END