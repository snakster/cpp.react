
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_ENGINE_SUBTREEENGINE_H_INCLUDED
#define REACT_DETAIL_ENGINE_SUBTREEENGINE_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <atomic>
#include <type_traits>
#include <vector>

#include "tbb/spin_rw_mutex.h"
#include "tbb/task.h"

#include "react/common/Containers.h"
#include "react/common/TopoQueue.h"
#include "react/common/Types.h"
#include "react/detail/EngineBase.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/
namespace subtree {

using std::atomic;
using std::vector;

using tbb::task;
using tbb::empty_task;
using tbb::task_list;
using tbb::spin_rw_mutex;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Turn
///////////////////////////////////////////////////////////////////////////////////////////////////
class Turn : public TurnBase
{
public:
    Turn(TurnIdT id, TransactionFlagsT flags);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Node
///////////////////////////////////////////////////////////////////////////////////////////////////
class Node : public IReactiveNode
{
public:
    using ShiftMutexT = spin_rw_mutex;

    inline bool IsQueued() const    { return flags_.Test<flag_queued>(); }
    inline void SetQueuedFlag()     { flags_.Set<flag_queued>(); }
    inline void ClearQueuedFlag()   { flags_.Clear<flag_queued>(); }

    inline bool IsMarked() const    { return flags_.Test<flag_marked>(); }
    inline void SetMarkedFlag()     { flags_.Set<flag_marked>(); }
    inline void ClearMarkedFlag()   { flags_.Clear<flag_marked>(); }

    inline bool IsChanged() const   { return flags_.Test<flag_changed>(); }
    inline void SetChangedFlag()    { flags_.Set<flag_changed>(); }
    inline void ClearChangedFlag()  { flags_.Clear<flag_changed>(); }

    inline bool IsDeferred() const   { return flags_.Test<flag_deferred>(); }
    inline void SetDeferredFlag()    { flags_.Set<flag_deferred>(); }
    inline void ClearDeferredFlag()  { flags_.Clear<flag_deferred>(); }

    inline bool IsRepeated() const   { return flags_.Test<flag_repeated>(); }
    inline void SetRepeatedFlag()    { flags_.Set<flag_repeated>(); }
    inline void ClearRepeatedFlag()  { flags_.Clear<flag_repeated>(); }

    inline bool IsInitial() const   { return flags_.Test<flag_initial>(); }
    inline void SetInitialFlag()    { flags_.Set<flag_initial>(); }
    inline void ClearInitialFlag()  { flags_.Clear<flag_initial>(); }

    inline bool IsRoot() const      { return flags_.Test<flag_root>(); }
    inline void SetRootFlag()       { flags_.Set<flag_root>(); }
    inline void ClearRootFlag()     { flags_.Clear<flag_root>(); }

    inline bool ShouldUpdate() const { return shouldUpdate_.load(std::memory_order_relaxed); }
    inline void SetShouldUpdate(bool b) { shouldUpdate_.store(b, std::memory_order_relaxed); }

    inline void SetReadyCount(int c)
    {
        readyCount_.store(c, std::memory_order_relaxed);
    }

    inline bool IncReadyCount()
    {
        auto t = readyCount_.fetch_add(1, std::memory_order_relaxed);
        return t < (WaitCount - 1);
    }

    inline bool DecReadyCount()
    {
        return readyCount_.fetch_sub(1, std::memory_order_relaxed) > 1;
    }

    NodeVector<Node>    Successors;
    ShiftMutexT         ShiftMutex;
    uint16_t            Level       = 0;
    uint16_t            NewLevel    = 0;
    uint16_t            WaitCount   = 0;

private:
    enum EFlags : uint16_t
    {
        flag_queued = 0,
        flag_marked,
        flag_changed,
        flag_deferred,
        flag_repeated,
        flag_initial,
        flag_root
    };

    EnumFlags<EFlags>   flags_;
    atomic<uint16_t>    readyCount_     { 0 };
    atomic<bool>        shouldUpdate_   { false };
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Functors
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct GetLevelFunctor
{
    int operator()(const T* x) const { return x->Level; }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EngineBase
///////////////////////////////////////////////////////////////////////////////////////////////////
class EngineBase : public IReactiveEngine<Node,Turn>
{
public:
    using TopoQueueT = TopoQueue<Node*, GetLevelFunctor<Node>>;
    using NodeShiftMutexT = Node::ShiftMutexT;

    void OnNodeAttach(Node& node, Node& parent);
    void OnNodeDetach(Node& node, Node& parent);

    void OnInputChange(Node& node, Turn& turn);
    void Propagate(Turn& turn);

    void OnNodePulse(Node& node, Turn& turn);
    void OnNodeIdlePulse(Node& node, Turn& turn);

    void OnDynamicNodeAttach(Node& node, Node& parent, Turn& turn);
    void OnDynamicNodeDetach(Node& node, Node& parent, Turn& turn);

private:
    void applyAsyncDynamicAttach(Node& node, Node& parent, Turn& turn);
    void applyAsyncDynamicDetach(Node& node, Node& parent, Turn& turn);

    void invalidateSuccessors(Node& node);
    void processChildren(Node& node, Turn& turn);

    void markSubtree(Node& root);

    TopoQueueT      scheduledNodes_;
    vector<Node*>   subtreeRoots_;

    empty_task&     rootTask_   = *new(task::allocate_root()) empty_task;
    task_list       spawnList_;

    bool            isInPhase2_ = false;
}; 

} // ~namespace subtree
/****************************************/ REACT_IMPL_END /***************************************/

/*****************************************/ REACT_BEGIN /*****************************************/

template <REACT_IMPL::EPropagationMode>
class SubtreeEngine;

template <>
class SubtreeEngine<REACT_IMPL::parallel_propagation> :
    public REACT_IMPL::subtree::EngineBase
{};

/******************************************/ REACT_END /******************************************/

/***************************************/ REACT_IMPL_BEGIN /**************************************/

template <>
struct NodeUpdateTimerEnabled<SubtreeEngine<parallel_propagation>> : std::true_type {};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_ENGINE_SUBTREEENGINE_H_INCLUDED