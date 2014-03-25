
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/Defs.h"

#include <vector>

#include "tbb/queuing_mutex.h"
#include "tbb/spin_mutex.h"
#include "tbb/task_group.h"

#include "EngineBase.h"
#include "react/ReactiveDomain.h"
#include "react/common/Containers.h"
#include "react/common/Util.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/
namespace flooding {

using std::atomic;
using std::set;
using std::vector;
using tbb::queuing_mutex;
using tbb::task_group;
using tbb::spin_mutex;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Turn
///////////////////////////////////////////////////////////////////////////////////////////////////
class Turn : public TurnBase
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
    using ShiftMutexT = spin_mutex;

    Node();

    bool    MarkForSchedule();
    bool    Evaluate(Turn& turn);

    NodeVector<Node>    Successors;
    ShiftMutexT            ShiftMutex;

private:
    using EvalMutexT = spin_mutex;

    atomic<bool>    isScheduled_;
    EvalMutexT      mutex_;

    bool    shouldReprocess_;
    bool    isProcessing_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EngineBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurn>
class EngineBase : public IReactiveEngine<Node,TTurn>
{
public:
    void OnNodeAttach(Node& node, Node& parent);
    void OnNodeDetach(Node& node, Node& parent);

    void OnTurnInputChange(Node& node, TTurn& turn);
    void OnTurnPropagate(TTurn& turn);

    void OnNodePulse(Node& node, TTurn& turn);

    void OnDynamicNodeAttach(Node& node, Node& parent, TTurn& turn);
    void OnDynamicNodeDetach(Node& node, Node& parent, TTurn& turn);

private:
    using OutputMutexT = queuing_mutex;
    using NodeShiftMutexT = Node::ShiftMutexT;

    set<Node*>      outputNodes_;
    OutputMutexT    outputMutex_;
    vector<Node*>   changedInputs_;

    task_group      tasks_;

    void pulse(Node& node, TTurn& turn);
    void process(Node& node, TTurn& turn);
};

class BasicEngine : public EngineBase<Turn> {};
class QueuingEngine : public DefaultQueuingEngine<EngineBase,Turn> {};

} // ~namespace flooding
/****************************************/ REACT_IMPL_END /***************************************/

/*****************************************/ REACT_BEGIN /*****************************************/

struct parallel;
struct parallel_queuing;

template <typename TMode>
class FloodingEngine;

template <> class FloodingEngine<parallel> : public REACT_IMPL::flooding::BasicEngine {};
template <> class FloodingEngine<parallel_queuing> : public REACT_IMPL::flooding::QueuingEngine {};

/******************************************/ REACT_END /******************************************/
