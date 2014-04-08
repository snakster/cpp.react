
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

using tbb::task_group;
using tbb::task;
using tbb::empty_task;
using tbb::spin_rw_mutex;

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
class Node : public IReactiveNode
{
public:
    using ShiftMutexT = spin_rw_mutex;

    inline void IncCounter() { counter_.fetch_add(1, std::memory_order_relaxed); }
    inline bool DecCounter() { return counter_.fetch_sub(1, std::memory_order_relaxed) > 1; }
    inline void SetCounter(int c) { counter_.store(c, std::memory_order_relaxed); }

    ShiftMutexT         ShiftMutex;
    NodeVector<Node>    Successors;

    atomic<bool>    ShouldUpdate = false;
    atomic<bool>    Marked = false;

private:
    atomic<int>     counter_ = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EngineBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurn>
class EngineBase : public IReactiveEngine<Node,TTurn>
{
public:
    using NodeShiftMutexT = Node::ShiftMutexT;
    using NodeVectorT = vector<Node*>;

    void OnNodeAttach(Node& node, Node& parent);
    void OnNodeDetach(Node& node, Node& parent);

    void OnTurnInputChange(Node& node, TTurn& turn);
    void OnTurnPropagate(TTurn& turn);

    void OnNodePulse(Node& node, TTurn& turn);
    void OnNodeIdlePulse(Node& node, TTurn& turn);

    void OnDynamicNodeAttach(Node& node, Node& parent, TTurn& turn);
    void OnDynamicNodeDetach(Node& node, Node& parent, TTurn& turn);

private:
    void processChild(Node& node, bool update, TTurn& turn);
    void nudgeChildren(Node& parent, bool update, TTurn& turn);

    vector<Node*>   changedInputs_;
    task_group      tasks_;

    empty_task*     rootTask_ = new( task::allocate_root() ) empty_task;
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

template <> class PulseCountEngine<parallel> : public REACT_IMPL::pulsecount::BasicEngine {};
template <> class PulseCountEngine<parallel_queuing> : public REACT_IMPL::pulsecount::QueuingEngine {};

/******************************************/ REACT_END /******************************************/
