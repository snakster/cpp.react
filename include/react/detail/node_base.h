
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_NODE_BASE_H_INCLUDED
#define REACT_DETAIL_NODE_BASE_H_INCLUDED

#pragma once

#include "react/detail/defs.h"

#include <iterator>
#include <memory>
#include <utility>

#include "react/common/utility.h"
#include "react/detail/graph_interface.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

class ReactGraph;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// CreateWrappedNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename RET, typename NODE, typename ... ARGS>
static RET CreateWrappedNode(ARGS&& ... args)
{
    auto node = std::make_shared<NODE>(std::forward<ARGS>(args) ...);
    return RET(std::move(node));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// NodeBase
///////////////////////////////////////////////////////////////////////////////////////////////////
class NodeBase : public IReactNode
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
    auto GetGraphPtr() const -> const std::shared_ptr<ReactGraph>&
        { return GetInternals(group_).GetGraphPtr(); }

    auto GetGraphPtr() -> std::shared_ptr<ReactGraph>&
        { return GetInternals(group_).GetGraphPtr(); }

    void RegisterMe(NodeCategory category = NodeCategory::normal)
        { nodeId_ = GetGraphPtr()->RegisterNode(this, category); }
    
    void UnregisterMe()
        { GetGraphPtr()->UnregisterNode(nodeId_); }

    void AttachToMe(NodeId otherNodeId)
        { GetGraphPtr()->AttachNode(nodeId_, otherNodeId); }

    void DetachFromMe(NodeId otherNodeId)
        { GetGraphPtr()->DetachNode(nodeId_, otherNodeId); }

private:
    NodeId nodeId_;

    Group group_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_NODE_BASE_H_INCLUDED