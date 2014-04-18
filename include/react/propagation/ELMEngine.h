
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/Defs.h"

#include <atomic>
#include <set>

#include "tbb/task_group.h"
#include "tbb/spin_mutex.h"

#include "react/ReactiveDomain.h"
#include "react/common/Containers.h"
#include "react/common/Types.h"
#include "react/propagation/EngineBase.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/
namespace elm {

using std::atomic;
using std::set;
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

    ShiftMutexT         ShiftMutex;
    NodeVector<Node>    Successors;

    atomic<int16_t>     Counter;
    atomic<bool>        ShouldUpdate;

    TurnIdT             LastTurnId;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Engine
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurn>
class EngineBase : public IReactiveEngine<Node,TTurn>
{
public:
    using NodeShiftMutexT = Node::ShiftMutexT;
    using NodeSetT = set<Node*>;

    void OnNodeCreate(Node& node);
    void OnNodeDestroy(Node& node);

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

    task_group  tasks_;
    NodeSetT    inputNodes_;
};

class BasicEngine : public EngineBase<Turn> {};
class QueuingEngine : public DefaultQueuingEngine<EngineBase,Turn> {};

} // ~namespace elm
/****************************************/ REACT_IMPL_END /***************************************/

/*****************************************/ REACT_BEGIN /*****************************************/

struct parallel;
struct parallel_queue;

template <typename TMode>
class ELMEngine;

template <> class ELMEngine<parallel> : public REACT_IMPL::elm::BasicEngine {};
template <> class ELMEngine<parallel_queue> : public REACT_IMPL::elm::QueuingEngine {};

/******************************************/ REACT_END /******************************************/
