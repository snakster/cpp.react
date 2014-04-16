
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/Defs.h"

#include <atomic>
#include <vector>

#include "tbb/task_group.h"
#include "tbb/spin_rw_mutex.h"
#include "tbb/task.h"

#include "EngineBase.h"
#include "react/ReactiveDomain.h"
#include "react/common/Containers.h"
#include "react/common/Types.h"

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
struct Turn : public TurnBase
{
public:
    Turn(TurnIdT id, TurnFlagsT flags);
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
    deferred
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
    uint                Weight      = 0;

private:
    atomic<int>         counter_    = 0;
    atomic<ENodeMark>   mark_       = ENodeMark::unmarked;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EngineBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurn>
class EngineBase : public IReactiveEngine<Node,TTurn>
{
public:
    using NodeShiftMutexT = Node::ShiftMutexT;
    using NodeVectT = vector<Node*>;

    void OnNodeAttach(Node& node, Node& parent);
    void OnNodeDetach(Node& node, Node& parent);

    void OnTurnInputChange(Node& node, TTurn& turn);
    void OnTurnPropagate(TTurn& turn);

    void OnNodePulse(Node& node, TTurn& turn);
    void OnNodeIdlePulse(Node& node, TTurn& turn);

    void OnDynamicNodeAttach(Node& node, Node& parent, TTurn& turn);
    void OnDynamicNodeDetach(Node& node, Node& parent, TTurn& turn);

    void HintUpdateDuration(Node& node, uint dur);

private:
    NodeVectT       changedInputs_;
    empty_task*     rootTask_ = new(task::allocate_root()) empty_task;
    task_list       spawnList_;
};

class BasicEngine : public EngineBase<Turn> {};
class QueuingEngine : public DefaultQueuingEngine<EngineBase,Turn> {};

} // ~namespace pulsecount
/****************************************/ REACT_IMPL_END /***************************************/

/*****************************************/ REACT_BEGIN /*****************************************/

struct parallel;
struct parallel_queuing;

template <typename TMode>
class PulseCountEngine;

template <> class PulseCountEngine<parallel> :
    public REACT_IMPL::pulsecount::BasicEngine {};

template <> class PulseCountEngine<parallel_queuing> :
    public REACT_IMPL::pulsecount::QueuingEngine {};

/******************************************/ REACT_END /******************************************/

/***************************************/ REACT_IMPL_BEGIN /**************************************/

template <typename T>
struct EnableNodeUpdateTimer;

template <>
struct EnableNodeUpdateTimer<PulseCountEngine<parallel>> : std::true_type {};

template <>
struct EnableNodeUpdateTimer<PulseCountEngine<parallel_queuing>> : std::true_type {};

/****************************************/ REACT_IMPL_END /***************************************/