
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
/// Forward declarations
///////////////////////////////////////////////////////////////////////////////////////////////////
class AsyncState;

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

    template <typename F>
    bool TryMergeSync(F&& f) { return false; }

    template <typename F>
    bool TryMergeAsync(F&& f, std::shared_ptr<AsyncState>&& statusPtr) { return false; }

    void ApplyMergedInputs(TurnT& turn) {}

    void EnterTurnQueue(TurnT& turn)    {}
    void ExitTurnQueue(TurnT& turn)     {}

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

    template <typename F>
    static bool TryMergeSync(F&& f)
    {
        return Instance().TryMergeSync(std::forward<F>(f));
    }

    template <typename F>
    static bool TryMergeAsync(F&& f, std::shared_ptr<AsyncState>&& statusPtr)
    {
        return Instance().TryMergeAsync(std::forward<F>(f), std::move(statusPtr));
    }

    static void ApplyMergedInputs(TurnT& turn)
    {
        Instance().ApplyMergedInputs(turn);
    }

    static void EnterTurnQueue(TurnT& turn)
    {
        REACT_LOG(D::Log().template Append<TransactionBeginEvent>(turn.Id()));
        Instance().EnterTurnQueue(turn);
    }

    static void ExitTurnQueue(TurnT& turn)
    {
        REACT_LOG(D::Log().template Append<TransactionEndEvent>(turn.Id()));
        Instance().ExitTurnQueue(turn);
    }

    static void OnTurnAdmissionStart(TurnT& turn)
    {
        Instance().OnTurnAdmissionEnd(turn);
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
template <typename> struct IsParallelEngine : std::false_type {};
template <typename> struct IsConcurrentEngine : std::false_type {};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_IREACTIVEENGINE_H_INCLUDED