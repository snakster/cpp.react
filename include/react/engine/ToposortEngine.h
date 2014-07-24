
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_ENGINE_TOPOSORTENGINE_H_INCLUDED
#define REACT_DETAIL_ENGINE_TOPOSORTENGINE_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <atomic>
#include <utility>
#include <type_traits>
#include <vector>

#include "tbb/concurrent_vector.h"
#include "tbb/spin_mutex.h"

#include "react/common/Containers.h"
#include "react/common/TopoQueue.h"
#include "react/common/Types.h"
#include "react/detail/EngineBase.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

namespace toposort {

using std::atomic;
using std::vector;
using tbb::concurrent_vector;
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
class SeqTurn : public TurnBase
{
public:
    SeqTurn(TurnIdT id, TransactionFlagsT flags);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ExclusiveParTurn
///////////////////////////////////////////////////////////////////////////////////////////////////
class ParTurn : public TurnBase
{
public:
    ParTurn(TurnIdT id, TransactionFlagsT flags);
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

    void OnInputChange(TNode& node, TTurn& turn);
    void OnNodePulse(TNode& node, TTurn& turn);

protected:
    virtual void processChildren(TNode& node, TTurn& turn) = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SeqEngineBase
///////////////////////////////////////////////////////////////////////////////////////////////////
class SeqEngineBase : public EngineBase<SeqNode,SeqTurn>
{
public:
    using TopoQueueT = TopoQueue<SeqNode*, GetLevelFunctor<SeqNode>>;

    void Propagate(SeqTurn& turn);

    void OnDynamicNodeAttach(SeqNode& node, SeqNode& parent, SeqTurn& turn);
    void OnDynamicNodeDetach(SeqNode& node, SeqNode& parent, SeqTurn& turn);

private:
    void invalidateSuccessors(SeqNode& node);

    virtual void processChildren(SeqNode& node, SeqTurn& turn) override;

    TopoQueueT    scheduledNodes_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ParEngineBase
///////////////////////////////////////////////////////////////////////////////////////////////////
class ParEngineBase : public EngineBase<ParNode,ParTurn>
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

    void Propagate(ParTurn& turn);

    void OnDynamicNodeAttach(ParNode& node, ParNode& parent, ParTurn& turn);
    void OnDynamicNodeDetach(ParNode& node, ParNode& parent, ParTurn& turn);

private:
    void applyDynamicAttach(ParNode& node, ParNode& parent, ParTurn& turn);
    void applyDynamicDetach(ParNode& node, ParNode& parent, ParTurn& turn);

    void invalidateSuccessors(ParNode& node);

    virtual void processChildren(ParNode& node, ParTurn& turn) override;

    TopoQueueT          topoQueue_;
    DynRequestVectT     dynRequests_;
};

} // ~namespace toposort

/****************************************/ REACT_IMPL_END /***************************************/

/*****************************************/ REACT_BEGIN /*****************************************/

template <REACT_IMPL::EPropagationMode>
class ToposortEngine;

template <> class ToposortEngine<REACT_IMPL::sequential_propagation> :
    public REACT_IMPL::toposort::SeqEngineBase
{};

template <> class ToposortEngine<REACT_IMPL::parallel_propagation> :
    public REACT_IMPL::toposort::ParEngineBase
{};

/******************************************/ REACT_END /******************************************/

/***************************************/ REACT_IMPL_BEGIN /**************************************/

template <>
struct NodeUpdateTimerEnabled<ToposortEngine<parallel_propagation>> : std::true_type {};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_ENGINE_TOPOSORTENGINE_H_INCLUDED