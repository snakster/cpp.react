
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <utility>

#include "react/detail/Defs.h"
#include "react/common/Types.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IReactiveEngine
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename TNode,
    typename TTurn
>
struct IReactiveEngine
{
    using NodeT = TNode;
    using TurnT = TTurn;

    void OnNodeCreate(NodeT& node)  {}
    void OnNodeDestroy(NodeT& node) {}

    void OnNodeAttach(NodeT& node, NodeT& parent)   {}
    void OnNodeDetach(NodeT& node, NodeT& parent)   {}

    void OnTurnAdmissionStart(TurnT& turn)  {}
    void OnTurnAdmissionEnd(TurnT& turn)    {}
    void OnTurnEnd(TurnT& turn)             {}

    void OnTurnInputChange(NodeT& node, TurnT& turn)    {}
    void OnTurnPropagate(TurnT& turn)                   {}

    void OnNodePulse(NodeT& node, TurnT& turn)      {}
    void OnNodeIdlePulse(NodeT& node, TurnT& turn)  {}

    void OnDynamicNodeAttach(NodeT& node, NodeT& parent, TurnT& turn)    {}
    void OnDynamicNodeDetach(NodeT& node, NodeT& parent, TurnT& turn)    {}

    template <typename F>
    bool TryMerge(F&& f) { return false; }

    void HintUpdateDuration(NodeT& node, uint dur)    {}
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EngineInterface - Static wrapper for IReactiveEngine
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename TEngine
>
struct EngineInterface
{
    using NodeT = typename TEngine::NodeT;
    using TurnT = typename TEngine::TurnT;

    static TEngine& Engine()
    {
        static TEngine engine;
        return engine;
    }

    static void OnNodeCreate(NodeT& node)
    {
        REACT_LOG(D::Log().template Append<NodeCreateEvent>(
            GetObjectId(node), node.GetNodeType()));
        Engine().OnNodeCreate(node);
    }

    static void OnNodeDestroy(NodeT& node)
    {
        REACT_LOG(D::Log().template Append<NodeDestroyEvent>(
            GetObjectId(node)));
        Engine().OnNodeDestroy(node);
    }

    static void OnNodeAttach(NodeT& node, NodeT& parent)
    {
        REACT_LOG(D::Log().template Append<NodeAttachEvent>(
            GetObjectId(node), GetObjectId(parent)));
        Engine().OnNodeAttach(node, parent);
    }

    static void OnNodeDetach(NodeT& node, NodeT& parent)
    {
        REACT_LOG(D::Log().template Append<NodeDetachEvent>(
            GetObjectId(node), GetObjectId(parent)));
        Engine().OnNodeDetach(node, parent);
    }

    static void OnNodePulse(NodeT& node, TurnT& turn)
    {
        REACT_LOG(D::Log().template Append<NodePulseEvent>(
            GetObjectId(node), turn.Id()));
        Engine().OnNodePulse(node, turn);
    }

    static void OnNodeIdlePulse(NodeT& node, TurnT& turn)
    {
        REACT_LOG(D::Log().template Append<NodeIdlePulseEvent>(
            GetObjectId(node), turn.Id()));
        Engine().OnNodeIdlePulse(node, turn);
    }

    static void OnDynamicNodeAttach(NodeT& node, NodeT& parent, TurnT& turn)
    {
        REACT_LOG(D::Log().template Append<DynamicNodeAttachEvent>(
            GetObjectId(node), GetObjectId(parent), turn.Id()));
        Engine().OnDynamicNodeAttach(node, parent, turn);
    }

    static void OnDynamicNodeDetach(NodeT& node, NodeT& parent, TurnT& turn)
    {
        REACT_LOG(D::Log().template Append<DynamicNodeDetachEvent>(
            GetObjectId(node), GetObjectId(parent), turn.Id()));
        Engine().OnDynamicNodeDetach(node, parent, turn);
    }

    static void OnTurnAdmissionStart(TurnT& turn)
    {
        REACT_LOG(D::Log().template Append<TransactionBeginEvent>(turn.Id()));
        Engine().OnTurnAdmissionStart(turn);
    }

    static void OnTurnAdmissionEnd(TurnT& turn)
    {
        Engine().OnTurnAdmissionEnd(turn);
    }

    static void OnTurnEnd(TurnT& turn)
    {
        REACT_LOG(D::Log().template Append<TransactionEndEvent>(turn.Id()));
        Engine().OnTurnEnd(turn);
    }

    static void OnTurnInputChange(NodeT& node, TurnT& turn)
    {
        REACT_LOG(D::Log().template Append<InputNodeAdmissionEvent>(
            GetObjectId(node), turn.Id()));
        Engine().OnTurnInputChange(node, turn);
    }

    static void OnTurnPropagate(TurnT& turn)
    {
        Engine().OnTurnPropagate(turn);
    }

    template <typename F>
    static bool TryMerge(F&& f)
    {
        return Engine().TryMerge(std::forward<F>(f));
    }

    static void HintUpdateDuration(NodeT& node, uint dur)
    {
        Engine().HintUpdateDuration(node, dur);
    }

};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Engine traits
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename> struct EnableNodeUpdateTimer : std::false_type {};
template <typename> struct EnableParallelUpdating : std::false_type {};

/****************************************/ REACT_IMPL_END /***************************************/
