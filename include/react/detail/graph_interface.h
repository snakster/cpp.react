
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_GRAPH_INTERFACE_H_INCLUDED
#define REACT_DETAIL_GRAPH_INTERFACE_H_INCLUDED

#pragma once

#include "react/detail/defs.h"

#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>

#include "react/api.h"
#include "react/common/utility.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Definitions
///////////////////////////////////////////////////////////////////////////////////////////////////
using NodeId = size_t;
using TurnId = size_t;
using LinkId = size_t;

static NodeId invalid_node_id = (std::numeric_limits<size_t>::max)();
static TurnId invalid_turn_id = (std::numeric_limits<size_t>::max)();
static LinkId invalid_link_id = (std::numeric_limits<size_t>::max)();

enum class UpdateResult
{
    unchanged,
    changed,
    shifted
};

enum class NodeCategory
{
    normal,
    input,
    dyninput,
    output,
    linkoutput
};

class ReactGraph;
struct IReactNode;

using LinkOutputMap = std::unordered_map<ReactGraph*, std::vector<std::function<void()>>>;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IReactNode
///////////////////////////////////////////////////////////////////////////////////////////////////
struct IReactNode
{
    virtual ~IReactNode() = default;

    virtual UpdateResult Update(TurnId turnId) noexcept = 0;
    
    virtual void Clear() noexcept
        { }

    virtual void CollectOutput(LinkOutputMap& output)
        { }
};



/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_GRAPH_INTERFACE_H_INCLUDED