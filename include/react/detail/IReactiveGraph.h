
//          Copyright Sebastian Jeckel 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_IREACTIVEENGINE_H_INCLUDED
#define REACT_DETAIL_IREACTIVEENGINE_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <functional>
#include <memory>
#include <utility>

#include "react/API.h"
#include "react/common/Types.h"
#include "react/common/Util.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Definitions
///////////////////////////////////////////////////////////////////////////////////////////////////
using NodeId = size_t;
using TurnId = size_t;

static NodeId invalid_node_id = (std::numeric_limits<size_t>::max)();
static TurnId invalid_turn_id = (std::numeric_limits<size_t>::max)();

enum class UpdateResult
{
    unchanged,
    changed,
    shifted
};

struct IReactiveGraph;
struct IReactiveNode;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IReactiveGraph
///////////////////////////////////////////////////////////////////////////////////////////////////
struct IReactiveGraph
{
    virtual ~IReactiveGraph() = default;

    virtual NodeId RegisterNode(IReactiveNode* nodePtr) = 0;
    virtual void UnregisterNode(NodeId node) = 0;

    virtual void OnNodeAttach(NodeId nodeId, NodeId parentId) = 0;
    virtual void OnNodeDetach(NodeId nodeId, NodeId parentId) = 0;

    virtual void OnDynamicNodeAttach(NodeId nodeId, NodeId parentId, TurnId turn) = 0;
    virtual void OnDynamicNodeDetach(NodeId nodeId, NodeId parentId, TurnId turn) = 0;

    virtual void AddInput(NodeId nodeId, std::function<void()> inputCallback) = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IReactiveNode
///////////////////////////////////////////////////////////////////////////////////////////////////
struct IReactiveNode
{
    virtual ~IReactiveNode() = default;

    /// Returns unique type identifier
    virtual const char* GetNodeType() const = 0;

    // Note: Could get rid of this ugly ptr by adding a template parameter to the interface
    // But that would mean all engine nodes need that template parameter too - so rather cast
    virtual UpdateResult Update(TurnId turnId) = 0;  

    /// Input nodes can be manipulated externally.
    virtual bool IsInputNode() const = 0;

    /// Output nodes can't have any successors.
    virtual bool IsOutputNode() const = 0;

    /// Dynamic nodes may change in topology as a result of tick.
    virtual bool IsDynamicNode() const = 0;

    // Number of predecessors.
    virtual int GetDependencyCount() const = 0;
};

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