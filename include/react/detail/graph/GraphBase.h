
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

struct IReactiveGraph;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// NodeBase
///////////////////////////////////////////////////////////////////////////////////////////////////
class NodeBase : public IReactiveNode
{
public:
    NodeBase(const std::shared_ptr<IReactiveGraph>& graphPtr) :
        graphPtr_( graphPtr )
        { }
    
    NodeBase(const NodeBase&) = delete;
    NodeBase& operator=(const NodeBase&) = delete;

    NodeBase(NodeBase&&) = delete;
    NodeBase& operator=(NodeBase&&) = delete;
    
    virtual bool IsInputNode() const override
        { return false; }

    virtual bool IsOutputNode() const override
        { return false; }

    virtual bool IsDynamicNode() const override
        { return false; }

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

    auto GraphPtr() const -> const std::shared_ptr<IReactiveGraph>&
        { return graphPtr_; }

    auto GraphPtr() -> std::shared_ptr<IReactiveGraph>&
        { return graphPtr_; }

protected:
    void RegisterMe()
        { nodeId_ = graphPtr_->RegisterNode(this); }
    
    void UnregisterMe()
        { graphPtr_->UnregisterNode(nodeId_); }

    void AttachToMe(NodeId otherNodeId)
        { graphPtr_->OnNodeAttach(nodeId_, otherNodeId); }

    void DetachFromMe(NodeId otherNodeId)
        { graphPtr_->OnNodeDetach(nodeId_, otherNodeId); }

    void DynamicAttachToMe(NodeId otherNodeId, TurnId turnId)
        { graphPtr_->OnDynamicNodeAttach(nodeId_, otherNodeId, turnId); }

    void DynamicDetachFromMe(NodeId otherNodeId, TurnId turnId)
        { graphPtr_->OnDynamicNodeDetach(nodeId_, otherNodeId, turnId); }

private:
    NodeId          nodeId_;

    std::shared_ptr<IReactiveGraph> graphPtr_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_GRAPH_GRAPHBASE_H_INCLUDED