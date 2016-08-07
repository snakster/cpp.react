
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

enum class NodeFlags
{
    none,
    input,
    output,
    dynamic,
    buffered
};
REACT_DEFINE_BITMASK_OPERATORS(NodeFlags)

struct IReactiveGraph;
struct IReactiveNode;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IReactiveGraph
///////////////////////////////////////////////////////////////////////////////////////////////////
struct IReactiveGraph
{
    virtual ~IReactiveGraph() = default;

    virtual NodeId RegisterNode(IReactiveNode* nodePtr, NodeFlags flags) = 0;
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

    virtual const char* GetNodeType() const = 0;

    virtual UpdateResult Update(TurnId turnId) = 0;  

    virtual int GetDependencyCount() const = 0;

    virtual void ClearBuffer() = 0;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_IREACTIVEENGINE_H_INCLUDED