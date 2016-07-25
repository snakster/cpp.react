
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

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IReactiveEngine
///////////////////////////////////////////////////////////////////////////////////////////////////
struct IReactiveEngine
{
    using NodeId = uint;
    using TurnId = uint;

    void OnTurnAdmissionStart(TurnId& turn)  {}
    void OnTurnAdmissionEnd(TurnId& turn)    {}

    void OnInputChange(NodeId& node, TurnId& turn)    {}

    void Propagate(TurnId& turn)  {}

    void OnNodeCreate(NodeId& node)  {}
    void OnNodeDestroy(NodeId& node) {}

    void OnNodeAttach(NodeId node, NodeId parent)   {}
    void OnNodeDetach(NodeId node, NodeId parent)   {}

    void OnNodePulse(NodeId& node, TurnId& turn)      {}
    void OnNodeIdlePulse(NodeId& node, TurnId& turn)  {}

    void OnDynamicNodeAttach(NodeId& node, NodeId& parent, TurnId& turn)    {}
    void OnDynamicNodeDetach(NodeId& node, NodeId& parent, TurnId& turn)    {}
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