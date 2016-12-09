
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
#include <unordered_map>
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
using LinkId = size_t;

static NodeId invalid_node_id = (std::numeric_limits<size_t>::max)();
static TurnId invalid_turn_id = (std::numeric_limits<size_t>::max)();
static LinkId invalid_link_id = (std::numeric_limits<size_t>::max)();

enum class UpdateResult
{
    unchanged,
    changed
};

enum class NodeCategory
{
    normal,
    input,
    dyninput,
    output,
    linkoutput
};

class ReactiveGraph;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IReactiveNode
///////////////////////////////////////////////////////////////////////////////////////////////////
struct IReactiveNode
{
    virtual ~IReactiveNode() = default;

    virtual const char* GetNodeType() const = 0;

    virtual UpdateResult Update(TurnId turnId, int successorCount) = 0;

    virtual int GetDependencyCount() const = 0;
};

using LinkOutputMap = std::unordered_map<ReactiveGraph*, std::vector<std::function<void()>>>;

struct ILinkOutputNode : public IReactiveNode
{
    virtual void CollectOutput(LinkOutputMap& output) = 0;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_IREACTIVEENGINE_H_INCLUDED