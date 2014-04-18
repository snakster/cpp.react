
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/Defs.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <set>
#include <utility>
#include <type_traits>
#include <vector>

#include "tbb/concurrent_vector.h"
#include "tbb/queuing_rw_mutex.h"
#include "tbb/spin_mutex.h"

#include "EngineBase.h"
#include "react/common/Concurrency.h"
#include "react/common/Containers.h"
#include "react/common/TopoQueue.h"
#include "react/common/Types.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

class TurnBase;

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
using tbb::spin_mutex;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Parameters
///////////////////////////////////////////////////////////////////////////////////////////////////
static const uint min_weight = 1;
static const uint grain_size = 100;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SeqNode
///////////////////////////////////////////////////////////////////////////////////////////////////
class SeqNode : public IReactiveNode
{
public:
    int     Level = 0;
    int     NewLevel = 0;
    bool    Queued = false;

    NodeVector<SeqNode>    Successors;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ParNode
///////////////////////////////////////////////////////////////////////////////////////////////////
class ParNode : public IReactiveNode
{
public:
    using InvalidateMutexT = spin_mutex;

    int             Level = 0;
    int             NewLevel = 0;
    atomic<bool>    Collected = false;
    uint            Weight = 1;

    NodeVector<ParNode>     Successors;
    InvalidateMutexT        InvalidateMutex;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ShiftRequestData
///////////////////////////////////////////////////////////////////////////////////////////////////
struct DynRequestData
{
    bool        ShouldAttach;
    ParNode*    Node;
    ParNode*    Parent;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ExclusiveTurn
///////////////////////////////////////////////////////////////////////////////////////////////////

class ExclusiveTurn : public REACT_IMPL::TurnBase
{
public:
    ExclusiveTurn(TurnIdT id, TurnFlagsT flags);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EngineBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TNode, typename TTurn>
class EngineBase : public IReactiveEngine<TNode,TTurn>
{
public:
    void OnNodeAttach(TNode& node, TNode& parent);
    void OnNodeDetach(TNode& node, TNode& parent);

    void OnTurnInputChange(TNode& node, TTurn& turn);
    void OnNodePulse(TNode& node, TTurn& turn);

protected:
    virtual void processChildren(TNode& node, TTurn& turn) = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SeqEngineBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurn>
class SeqEngineBase : public EngineBase<SeqNode,TTurn>
{
public:
    using TopoQueueT = TopoQueue<SeqNode*>;

    void OnTurnPropagate(TTurn& turn);

    void OnDynamicNodeAttach(SeqNode& node, SeqNode& parent, TTurn& turn);
    void OnDynamicNodeDetach(SeqNode& node, SeqNode& parent, TTurn& turn);

private:
    void invalidateSuccessors(SeqNode& node);

    virtual void processChildren(SeqNode& node, TTurn& turn) override;

    TopoQueueT    scheduledNodes_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ParEngineBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurn>
class ParEngineBase : public EngineBase<ParNode,TTurn>
{
public:
    using DynRequestVectT = concurrent_vector<DynRequestData>;
    using TopoQueueT = ConcurrentTopoQueue<ParNode*,grain_size>;

    void OnTurnPropagate(TTurn& turn);

    void OnDynamicNodeAttach(ParNode& node, ParNode& parent, TTurn& turn);
    void OnDynamicNodeDetach(ParNode& node, ParNode& parent, TTurn& turn);

    void HintUpdateDuration(ParNode& node, uint dur);

private:
    void applyDynamicAttach(ParNode& node, ParNode& parent, TTurn& turn);
    void applyDynamicDetach(ParNode& node, ParNode& parent, TTurn& turn);

    void invalidateSuccessors(ParNode& node);

    virtual void processChildren(ParNode& node, TTurn& turn) override;

    TopoQueueT          topoQueue_;
    DynRequestVectT     dynRequests_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Concrete engines
///////////////////////////////////////////////////////////////////////////////////////////////////
class BasicSeqEngine : public SeqEngineBase<ExclusiveTurn> {};
class QueuingSeqEngine : public DefaultQueuingEngine<SeqEngineBase,ExclusiveTurn> {};

class BasicParEngine : public ParEngineBase<ExclusiveTurn> {};
class QueuingParEngine : public DefaultQueuingEngine<ParEngineBase,ExclusiveTurn> {};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// PipeliningTurn
///////////////////////////////////////////////////////////////////////////////////////////////////
class PipeliningTurn : public TurnBase
{
public:
    using ConcNodeVectT = concurrent_vector<ParNode*>;
    using NodeVectT = vector<ParNode*>;
    using IntervalSetT = set<pair<int,int>>;
    using DynRequestVectT = concurrent_vector<DynRequestData>;
    using TopoQueueT = ConcurrentTopoQueue<ParNode*,grain_size>;

    PipeliningTurn(TurnIdT id, TurnFlagsT flags);

    int     CurrentLevel() const    { return currentLevel_; }

    bool    AdvanceLevel();
    void    SetMaxLevel(int level);
    void    WaitForMaxLevel(int targetLevel);

    void    UpdateSuccessor();

    void    Append(PipeliningTurn* turn);

    void    Remove();

    void    AdjustUpperBound(int level);

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

    TopoQueueT          TopoQueue;
    DynRequestVectT     DynRequests;

private:
    using MergedDataVectT = vector<pair<function<void()>,BlockingCondition*>>;

    const bool          isMergeable_;
    MergedDataVectT     merged_;

    IntervalSetT        levelIntervals_;

    PipeliningTurn*     predecessor_ = nullptr;
    PipeliningTurn*     successor_ = nullptr;

    int     currentLevel_ = -1;
    int     maxLevel_ = INT_MAX;        /// This turn may only advance up to maxLevel
    int     minLevel_ = -1;             /// successor.maxLevel = this.minLevel - 1

    int     curUpperBound_ = -1;

    mutex               advMutex_;
    condition_variable  advCondition_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// PipeliningEngine
///////////////////////////////////////////////////////////////////////////////////////////////////
class PipeliningEngine : public IReactiveEngine<ParNode,PipeliningTurn>
{
public:
    using SeqMutexT = queuing_rw_mutex;
    using NodeSetT = set<ParNode*>;
    using MaxDynamicLevelMutexT = spin_mutex;

    void OnNodeAttach(ParNode& node, ParNode& parent);
    void OnNodeDetach(ParNode& node, ParNode& parent);

    void OnTurnAdmissionStart(PipeliningTurn& turn);
    void OnTurnAdmissionEnd(PipeliningTurn& turn);
    void OnTurnEnd(PipeliningTurn& turn);

    void OnTurnInputChange(ParNode& node, PipeliningTurn& turn);
    void OnNodePulse(ParNode& node, PipeliningTurn& turn);

    void OnTurnPropagate(PipeliningTurn& turn);

    void OnDynamicNodeAttach(ParNode& node, ParNode& parent, PipeliningTurn& turn);
    void OnDynamicNodeDetach(ParNode& node, ParNode& parent, PipeliningTurn& turn);

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
    void applyDynamicAttach(ParNode& node, ParNode& parent, PipeliningTurn& turn);
    void applyDynamicDetach(ParNode& node, ParNode& parent, PipeliningTurn& turn);

    void processChildren(ParNode& node, PipeliningTurn& turn);
    void invalidateSuccessors(ParNode& node);

    void advanceTurn(PipeliningTurn& turn);

    SeqMutexT           seqMutex_;
    PipeliningTurn*     tail_ = nullptr;

    NodeSetT    dynamicNodes_;
    int         maxDynamicLevel_;
    
    MaxDynamicLevelMutexT   maxDynamicLevelMutex_;
};

} // ~namespace toposort

/****************************************/ REACT_IMPL_END /***************************************/

/*****************************************/ REACT_BEGIN /*****************************************/

struct sequential;
struct sequential_queue;
struct parallel;
struct parallel_queue;
struct parallel_pipeline;

template <typename TMode>
class TopoSortEngine;

template <> class TopoSortEngine<sequential> :
    public REACT_IMPL::toposort::BasicSeqEngine {};

template <> class TopoSortEngine<sequential_queue> :
    public REACT_IMPL::toposort::QueuingSeqEngine {};

template <> class TopoSortEngine<parallel> :
    public REACT_IMPL::toposort::BasicParEngine {};

template <> class TopoSortEngine<parallel_queue> :
    public REACT_IMPL::toposort::QueuingParEngine {};

template <> class TopoSortEngine<parallel_pipeline> :
    public REACT_IMPL::toposort::PipeliningEngine {};

/******************************************/ REACT_END /******************************************/

/***************************************/ REACT_IMPL_BEGIN /**************************************/

template <typename T>
struct EnableNodeUpdateTimer;

template <>
struct EnableNodeUpdateTimer<TopoSortEngine<parallel>> : std::true_type {};

template <>
struct EnableNodeUpdateTimer<TopoSortEngine<parallel_queue>> : std::true_type {};

template <>
struct EnableNodeUpdateTimer<TopoSortEngine<parallel_pipeline>> : std::true_type {};

/****************************************/ REACT_IMPL_END /***************************************/
