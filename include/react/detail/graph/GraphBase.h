
//          Copyright Sebastian Jeckel 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_GRAPH_GRAPHBASE_H_INCLUDED
#define REACT_DETAIL_GRAPH_GRAPHBASE_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <iterator>
#include <memory>
#include <utility>

#include "react/common/Types.h"
#include "react/common/Util.h"
#include "react/detail/IReactiveGraph.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

class ReactiveGraph;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// NodeBase
///////////////////////////////////////////////////////////////////////////////////////////////////
class NodeBase : public IReactiveNode
{
public:
    NodeBase(const Group& group) :
        group_( group )
        { }
    
    NodeBase(const NodeBase&) = delete;
    NodeBase& operator=(const NodeBase&) = delete;

    NodeBase(NodeBase&&) = delete;
    NodeBase& operator=(NodeBase&&) = delete;

    /*void SetWeightHint(WeightHint weight)
    {
        switch (weight)
        {
        case WeightHint::heavy :
            this->ForceUpdateThresholdExceeded(true);
            break;
        case WeightHint::light :
            this->ForceUpdateThresholdExceeded(false);
            break;
        case WeightHint::automatic :
            this->ResetUpdateThreshold();
            break;
        }
    }*/

    NodeId GetNodeId() const
        { return nodeId_; }

    auto GetGroup() const -> const Group&
        { return group_; }

    auto GetGroup() -> Group&
        { return group_; }

protected:
    auto GetGraphPtr() const -> const std::shared_ptr<ReactiveGraph>&
        { return GetInternals(group_).GetGraphPtr(); }

    auto GetGraphPtr() -> std::shared_ptr<ReactiveGraph>&
        { return GetInternals(group_).GetGraphPtr(); }

    void RegisterMe(NodeCategory category = NodeCategory::normal)
        { nodeId_ = GetGraphPtr()->RegisterNode(this, category); }
    
    void UnregisterMe()
        { GetGraphPtr()->UnregisterNode(nodeId_); }

    void AttachToMe(NodeId otherNodeId)
        { GetGraphPtr()->OnNodeAttach(nodeId_, otherNodeId); }

    void DetachFromMe(NodeId otherNodeId)
        { GetGraphPtr()->OnNodeDetach(nodeId_, otherNodeId); }

private:
    NodeId nodeId_;

    Group group_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_GRAPH_GRAPHBASE_H_INCLUDED