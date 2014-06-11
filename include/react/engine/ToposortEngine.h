
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_ENGINE_TOPOSORTENGINE_H_INCLUDED
#define REACT_DETAIL_ENGINE_TOPOSORTENGINE_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <limits>
#include <mutex>
#include <set>
#include <utility>
#include <type_traits>
#include <vector>

#include "tbb/concurrent_vector.h"
#include "tbb/queuing_rw_mutex.h"
#include "tbb/spin_mutex.h"

#include "react/common/Concurrency.h"
#include "react/common/Containers.h"
#include "react/common/TopoQueue.h"
#include "react/common/Types.h"
#include "react/detail/EngineBase.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

template <bool is_thread_safe>
class TurnBase;

namespace toposort {

using std::atomic;
using std::condition_variable;
using std::function;
using std::mutex;
using std::numeric_limits;
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
    int     Level       { 0 };
    int     NewLevel    { 0 };
    bool    Queued      { false };

    NodeVector<SeqNode>     Successors;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ParNode
///////////////////////////////////////////////////////////////////////////////////////////////////
class ParNode : public IReactiveNode
{
public:
    using InvalidateMutexT = spin_mutex;

    int             Level       { 0 };
    int             NewLevel    { 0 };
    atomic<bool>    Collected   { false };

    NodeVector<ParNode> Successors;
    InvalidateMutexT    InvalidateMutex;
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
/// ExclusiveSeqTurn
///////////////////////////////////////////////////////////////////////////////////////////////////
class ExclusiveSeqTurn : public REACT_IMPL::TurnBase<false>
{
public:
    ExclusiveSeqTurn(TurnIdT id, TurnFlagsT flags);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ExclusiveParTurn
///////////////////////////////////////////////////////////////////////////////////////////////////
class ExclusiveParTurn : public REACT_IMPL::TurnBase<true>
{
public:
    ExclusiveParTurn(TurnIdT id, TurnFlagsT flags);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Functors
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct GetLevelFunctor
{
    int operator()(const T* x) const { return x->Level; }
};

template <typename T>
struct GetWeightFunctor
{
    uint operator()(T* x) const { return x->IsHeavyweight() ? grain_size : 1; }
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
    using TopoQueueT = TopoQueue<SeqNode*, GetLevelFunctor<SeqNode>>;

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
    using TopoQueueT = ConcurrentTopoQueue
    <
        ParNode*,
        grain_size,
        GetLevelFunctor<ParNode>,
        GetWeightFunctor<ParNode>
    >;

    void OnTurnPropagate(TTurn& turn);

    void OnDynamicNodeAttach(ParNode& node, ParNode& parent, TTurn& turn);
    void OnDynamicNodeDetach(ParNode& node, ParNode& parent, TTurn& turn);

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
class BasicSeqEngine : public SeqEngineBase<ExclusiveSeqTurn> {};
class QueuingSeqEngine : public DefaultQueuingEngine<SeqEngineBase,ExclusiveSeqTurn> {};

class BasicParEngine : public ParEngineBase<ExclusiveParTurn> {};
class QueuingParEngine : public DefaultQueuingEngine<ParEngineBase,ExclusiveParTurn> {};

} // ~namespace toposort

/****************************************/ REACT_IMPL_END /***************************************/

/*****************************************/ REACT_BEGIN /*****************************************/

struct sequential;
struct sequential_queue;
struct parallel;
struct parallel_queue;

template <typename TMode>
class ToposortEngine;

template <> class ToposortEngine<sequential> :
    public REACT_IMPL::toposort::BasicSeqEngine {};

template <> class ToposortEngine<sequential_queue> :
    public REACT_IMPL::toposort::QueuingSeqEngine {};

template <> class ToposortEngine<parallel> :
    public REACT_IMPL::toposort::BasicParEngine {};

template <> class ToposortEngine<parallel_queue> :
    public REACT_IMPL::toposort::QueuingParEngine {};

/******************************************/ REACT_END /******************************************/

/***************************************/ REACT_IMPL_BEGIN /**************************************/

template <typename> struct EnableNodeUpdateTimer;
template <> struct EnableNodeUpdateTimer<ToposortEngine<parallel>> : std::true_type {};
template <> struct EnableNodeUpdateTimer<ToposortEngine<parallel_queue>> : std::true_type {};

template <typename> struct EnableParallelUpdating;
template <> struct EnableParallelUpdating<ToposortEngine<parallel>> : std::true_type {};
template <> struct EnableParallelUpdating<ToposortEngine<parallel_queue>> : std::true_type {};

template <typename> struct EnableConcurrentInput;
template <> struct EnableConcurrentInput<ToposortEngine<sequential_queue>> : std::true_type {};
template <> struct EnableConcurrentInput<ToposortEngine<parallel_queue>> : std::true_type {};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_ENGINE_TOPOSORTENGINE_H_INCLUDED