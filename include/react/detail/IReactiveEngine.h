
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_IREACTIVEENGINE_H_INCLUDED
#define REACT_DETAIL_IREACTIVEENGINE_H_INCLUDED

#pragma once

#include <memory>
#include <utility>

#include "react/detail/Defs.h"
#include "react/common/Types.h"
#include "react/logging/EventRecords.h"

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

    void OnTurnAdmissionStart(TurnT& turn)  {}
    void OnTurnAdmissionEnd(TurnT& turn)    {}

    void OnInputChange(NodeT& node, TurnT& turn)    {}

    void Propagate(TurnT& turn)  {}

    void OnNodeCreate(NodeT& node)  {}
    void OnNodeDestroy(NodeT& node) {}

    void OnNodeAttach(NodeT& node, NodeT& parent)   {}
    void OnNodeDetach(NodeT& node, NodeT& parent)   {}

    void OnNodePulse(NodeT& node, TurnT& turn)      {}
    void OnNodeIdlePulse(NodeT& node, TurnT& turn)  {}

    void OnDynamicNodeAttach(NodeT& node, NodeT& parent, TurnT& turn)    {}
    void OnDynamicNodeDetach(NodeT& node, NodeT& parent, TurnT& turn)    {}
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

    static TEngine& Instance()
    {
        static TEngine engine;
        return engine;
    }

    static void OnTurnAdmissionStart(TurnT& turn)
    {
        Instance().OnTurnAdmissionStart(turn);
    }

    static void OnTurnAdmissionEnd(TurnT& turn)
    {
        Instance().OnTurnAdmissionEnd(turn);
    }

    static void OnInputChange(NodeT& node, TurnT& turn)
    {
        REACT_LOG(D::Log().template Append<InputNodeAdmissionEvent>(
            GetObjectId(node), turn.Id()));
        Instance().OnInputChange(node, turn);
    }

    static void Propagate(TurnT& turn)
    {
        Instance().Propagate(turn);
    }

    static void OnNodeCreate(NodeT& node)
    {
        REACT_LOG(D::Log().template Append<NodeCreateEvent>(
            GetObjectId(node), node.GetNodeType()));
        Instance().OnNodeCreate(node);
    }

    static void OnNodeDestroy(NodeT& node)
    {
        REACT_LOG(D::Log().template Append<NodeDestroyEvent>(
            GetObjectId(node)));
        Instance().OnNodeDestroy(node);
    }

    static void OnNodeAttach(NodeT& node, NodeT& parent)
    {
        REACT_LOG(D::Log().template Append<NodeAttachEvent>(
            GetObjectId(node), GetObjectId(parent)));
        Instance().OnNodeAttach(node, parent);
    }

    static void OnNodeDetach(NodeT& node, NodeT& parent)
    {
        REACT_LOG(D::Log().template Append<NodeDetachEvent>(
            GetObjectId(node), GetObjectId(parent)));
        Instance().OnNodeDetach(node, parent);
    }

    static void OnNodePulse(NodeT& node, TurnT& turn)
    {
        REACT_LOG(D::Log().template Append<NodePulseEvent>(
            GetObjectId(node), turn.Id()));
        Instance().OnNodePulse(node, turn);
    }

    static void OnNodeIdlePulse(NodeT& node, TurnT& turn)
    {
        REACT_LOG(D::Log().template Append<NodeIdlePulseEvent>(
            GetObjectId(node), turn.Id()));
        Instance().OnNodeIdlePulse(node, turn);
    }

    static void OnDynamicNodeAttach(NodeT& node, NodeT& parent, TurnT& turn)
    {
        REACT_LOG(D::Log().template Append<DynamicNodeAttachEvent>(
            GetObjectId(node), GetObjectId(parent), turn.Id()));
        Instance().OnDynamicNodeAttach(node, parent, turn);
    }

    static void OnDynamicNodeDetach(NodeT& node, NodeT& parent, TurnT& turn)
    {
        REACT_LOG(D::Log().template Append<DynamicNodeDetachEvent>(
            GetObjectId(node), GetObjectId(parent), turn.Id()));
        Instance().OnDynamicNodeDetach(node, parent, turn);
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Engine traits
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename> struct NodeUpdateTimerEnabled : std::false_type {};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EPropagationMode
///////////////////////////////////////////////////////////////////////////////////////////////////
enum EPropagationMode
{
    sequential_propagation,
    parallel_propagation
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_IREACTIVEENGINE_H_INCLUDED