
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/Defs.h"

#include <atomic>
#include <vector>

#include "tbb/queuing_mutex.h"
#include "tbb/spin_mutex.h"
#include "tbb/task_group.h"

#include "EngineBase.h"
#include "react/ReactiveDomain.h"
#include "react/common/Containers.h"
#include "react/common/SourceIdSet.h"
#include "react/common/Types.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/
namespace sourceset {

class Node;

using std::atomic;
using std::vector;
using tbb::task_group;
using tbb::queuing_mutex;
using tbb::spin_mutex;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Turn
///////////////////////////////////////////////////////////////////////////////////////////////////
class Turn : public TurnBase
{
public:
    using SourceIdSetT = SourceIdSet<ObjectId>;

    Turn(TurnIdT id, TurnFlagsT flags);

    void AddSourceId(ObjectId id);

    SourceIdSetT& Sources()        { return sources_; }

private:
    SourceIdSetT sources_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Node
///////////////////////////////////////////////////////////////////////////////////////////////////
class Node : public IReactiveNode
{
public:
    using SourceIdSetT = Turn::SourceIdSetT;

    using NudgeMutexT = queuing_mutex;
    using ShiftMutexT = spin_mutex;

    Node();

    void AddSourceId(ObjectId id);

    void AttachSuccessor(Node& node);
    void DetachSuccessor(Node& node);

    void DynamicAttachTo(Node& parent, Turn& turn);
    void DynamicDetachFrom(Node& parent, Turn& turn);

    void Destroy();

    void Pulse(Turn& turn, bool updated);

    bool IsDependency(Turn& turn);
    bool CheckCurrentTurn(Turn& turn);

    void Nudge(Turn& turn, bool update, bool invalidate);

    void CheckForCycles(ObjectId startId) const;

private:
    enum
    {
        kFlag_Visited =        1 << 0,
        kFlag_Updated =        1 << 1,
        kFlag_Invaliated =    1 << 2
    };

    NodeVector<Node>    predecessors_;
    NodeVector<Node>    successors_;

    SourceIdSetT    sources_;

    uint            curTurnId_;
    
    short           tickThreshold_;
    uchar           flags_;

    NudgeMutexT     nudgeMutex_;
    ShiftMutexT     shiftMutex_;

    void invalidateSources();
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EngineBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurn>
class EngineBase : public IReactiveEngine<Node,TTurn>
{
public:
    void OnNodeCreate(Node& node);

    void OnNodeAttach(Node& node, Node& parent);
    void OnNodeDetach(Node& node, Node& parent);

    void OnNodeDestroy(Node& node);

    void OnTurnInputChange(Node& node, TTurn& turn);
    void OnTurnPropagate(TTurn& turn);

    void OnNodePulse(Node& node, TTurn& turn);
    void OnNodeIdlePulse(Node& node, TTurn& turn);

    void OnDynamicNodeAttach(Node& node, Node& parent, TTurn& turn);
    void OnDynamicNodeDetach(Node& node, Node& parent, TTurn& turn);

private:
    vector<Node*>   changedInputs_;
};

class BasicEngine : public EngineBase<Turn> {};
class QueuingEngine : public DefaultQueuingEngine<EngineBase,Turn> {};

} // ~namespace sourceset
/****************************************/ REACT_IMPL_END /***************************************/

/*****************************************/ REACT_BEGIN /*****************************************/

struct parallel;
struct parallel_queuing;

template <typename TMode>
class SourceSetEngine;

template <> class SourceSetEngine<parallel> : public REACT_IMPL::sourceset::BasicEngine {};
template <> class SourceSetEngine<parallel_queuing> : public REACT_IMPL::sourceset::QueuingEngine {};

/******************************************/ REACT_END /******************************************/
