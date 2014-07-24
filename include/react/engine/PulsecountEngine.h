
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_ENGINE_PULSECOUNTENGINE_H_INCLUDED
#define REACT_DETAIL_ENGINE_PULSECOUNTENGINE_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <atomic>
#include <type_traits>
#include <vector>

#include "tbb/task_group.h"
#include "tbb/spin_rw_mutex.h"
#include "tbb/task.h"

#include "react/common/Containers.h"
#include "react/common/Types.h"
#include "react/detail/EngineBase.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/
namespace pulsecount {

using std::atomic;
using std::vector;

using tbb::task;
using tbb::empty_task;
using tbb::spin_rw_mutex;
using tbb::task_list;

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
enum class ENodeMark
{
    unmarked,
    visited,
    should_update
};

enum class ENodeState
{
    unchanged,
    changed,
    dyn_defer,
    dyn_repeat
};

class Node : public IReactiveNode
{
public:
    using ShiftMutexT = spin_rw_mutex;

    inline void IncCounter() { counter_.fetch_add(1, std::memory_order_relaxed); }
    inline bool DecCounter() { return counter_.fetch_sub(1, std::memory_order_relaxed) > 1; }
    inline void SetCounter(int c) { counter_.store(c, std::memory_order_relaxed); }

    inline ENodeMark Mark() const
    {
        return mark_.load(std::memory_order_relaxed);
    }

    inline void SetMark(ENodeMark mark)
    {
        mark_.store(mark, std::memory_order_relaxed);
    }

    inline bool ExchangeMark(ENodeMark mark)
    {
        return mark_.exchange(mark, std::memory_order_relaxed) != mark;
    }

    ShiftMutexT         ShiftMutex;
    NodeVector<Node>    Successors;
    
    ENodeState          State       = ENodeState::unchanged;

private:
    atomic<int>         counter_    { 0 };
    atomic<ENodeMark>   mark_       { ENodeMark::unmarked };
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EngineBase
///////////////////////////////////////////////////////////////////////////////////////////////////
class EngineBase : public IReactiveEngine<Node,Turn>
{
public:
    using NodeShiftMutexT = Node::ShiftMutexT;
    using NodeVectT = vector<Node*>;

    void OnNodeAttach(Node& node, Node& parent);
    void OnNodeDetach(Node& node, Node& parent);

    void OnInputChange(Node& node, Turn& turn);
    void Propagate(Turn& turn);

    void OnNodePulse(Node& node, Turn& turn);
    void OnNodeIdlePulse(Node& node, Turn& turn);

    void OnDynamicNodeAttach(Node& node, Node& parent, Turn& turn);
    void OnDynamicNodeDetach(Node& node, Node& parent, Turn& turn);

private:
    NodeVectT       changedInputs_;
    empty_task&     rootTask_       = *new(task::allocate_root()) empty_task;
    task_list       spawnList_;
};

} // ~namespace pulsecount
/****************************************/ REACT_IMPL_END /***************************************/

/*****************************************/ REACT_BEGIN /*****************************************/

template <REACT_IMPL::EPropagationMode>
class PulsecountEngine;

template <>
class PulsecountEngine<REACT_IMPL::parallel_propagation> :
    public REACT_IMPL::pulsecount::EngineBase
{};

/******************************************/ REACT_END /******************************************/

/***************************************/ REACT_IMPL_BEGIN /**************************************/

template <>
struct NodeUpdateTimerEnabled<PulsecountEngine<parallel_propagation>> : std::true_type {};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_ENGINE_PULSECOUNTENGINE_H_INCLUDED