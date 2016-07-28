
//          Copyright Sebastian Jeckel 2016.
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

using NodeId = uint;
using TurnId = uint;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IReactiveEngine
///////////////////////////////////////////////////////////////////////////////////////////////////
struct IReactiveGroup
{
    virtual ~IReactiveGroup() = 0;

    virtual void OnTurnAdmissionStart(TurnId turn) = 0;
    virtual void OnTurnAdmissionEnd(TurnId turn) = 0;

    virtual void OnInputChange(NodeId node, TurnId turn) = 0;

    virtual void Propagate(TurnId turn) = 0;

    virtual NodeId OnNodeCreate() = 0;
    virtual void OnNodeDestroy(NodeId node) = 0;

    virtual void OnNodeAttach(NodeId node, NodeId parent) = 0;
    virtual void OnNodeDetach(NodeId node, NodeId parent) = 0;

    virtual void OnDynamicNodeAttach(NodeId node, NodeId parent, TurnId turn) = 0;
    virtual void OnDynamicNodeDetach(NodeId node, NodeId parent, TurnId turn) = 0;
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