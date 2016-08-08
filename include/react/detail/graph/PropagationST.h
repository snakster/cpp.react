
//          Copyright Sebastian Jeckel 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_GRAPH_PROPAGATIONST_H_INCLUDED
#define REACT_DETAIL_GRAPH_PROPAGATIONST_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <algorithm>
#include <type_traits>
#include <utility>
#include <vector>

#include "react/common/Containers.h"
#include "react/common/Types.h"
#include "react/detail/IReactiveGraph.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

class SingleThreadedGraph : public IReactiveGraph
{
public:
    // IReactiveGraph
    virtual NodeId RegisterNode(IReactiveNode* nodePtr, NodeFlags flags) override;
    virtual void UnregisterNode(NodeId node) override;

    virtual void OnNodeAttach(NodeId node, NodeId parentId) override;
    virtual void OnNodeDetach(NodeId node, NodeId parentId) override;

    virtual void OnDynamicNodeAttach(NodeId node, NodeId parentId, TurnId turnId) override;
    virtual void OnDynamicNodeDetach(NodeId node, NodeId parentId, TurnId turnId) override;

    virtual void AddInput(NodeId nodeId, std::function<void()> inputCallback) override;
    // ~IReactiveGraph

    template <typename F>
    void DoTransaction(TransactionFlags flags, F&& transactionCallback);

private:
    struct NodeData
    {
        NodeData() = default;

        NodeData(const NodeData&) = default;
        NodeData& operator=(const NodeData&) = default;

        NodeData(IReactiveNode* nodePtrIn, NodeFlags flagsIn) :
            nodePtr( nodePtrIn ),
            flags( flagsIn )
        { }

        NodeFlags flags = NodeFlags::none;

        int     level       = 0;
        int     newLevel    = 0 ;
        bool    queued      = false;

        IReactiveNode*  nodePtr = nullptr;
        

        std::vector<NodeId> successors;
    };

    class TopoQueue
    {
    public:
        void Push(NodeId nodeId, int level)
            { queueData_.emplace_back(nodeId, level); }

        bool FetchNext();

        const std::vector<NodeId>& Next() const
            { return nextData_; }

        bool IsEmpty() const
            { return queueData_.empty(); }

    private:
        using Entry = std::pair<NodeId /*nodeId*/, int /*level*/>;

        std::vector<Entry>  queueData_;
        std::vector<NodeId> nextData_;

        int minLevel_ = (std::numeric_limits<int>::max)();
    };

    void Propagate();

    void ScheduleSuccessors(NodeData & node);
    void InvalidateSuccessors(NodeData & node);
    void ClearBufferedNodes();

private:
    int refCount_ = 1;

    TopoQueue           scheduledNodes_;
    IndexMap<NodeData>  nodeData_;
    std::vector<NodeId> changedInputs_;

    std::vector<IReactiveNode*> pendingBufferedNodes_;

    bool isTransactionActive_ = false;
};

NodeId SingleThreadedGraph::RegisterNode(IReactiveNode* nodePtr, NodeFlags flags)
{
    return nodeData_.Insert(NodeData{ nodePtr, flags });
}

void SingleThreadedGraph::UnregisterNode(NodeId nodeId)
{
    nodeData_.Remove(nodeId);

}

void SingleThreadedGraph::OnNodeAttach(NodeId nodeId, NodeId parentId)
{
    auto& node = nodeData_[nodeId];
    auto& parent = nodeData_[parentId];

    parent.successors.push_back(nodeId);

    if (node.level <= parent.level)
        node.level = parent.level + 1;
}

void SingleThreadedGraph::OnNodeDetach(NodeId nodeId, NodeId parentId)
{
    auto& parent = nodeData_[parentId];
    auto& successors = parent.successors;

    successors.erase(std::find(successors.begin(), successors.end(), nodeId));
}

void SingleThreadedGraph::OnDynamicNodeAttach(NodeId nodeId, NodeId parentId, TurnId turnId)
{
    OnNodeAttach(nodeId, parentId);
}

void SingleThreadedGraph::OnDynamicNodeDetach(NodeId nodeId, NodeId parentId, TurnId turnId)
{
    OnNodeDetach(nodeId, parentId);
}

void SingleThreadedGraph::AddInput(NodeId nodeId, std::function<void()> inputCallback)
{
    auto& node = nodeData_[nodeId];
    auto* nodePtr = node.nodePtr;

    // This write to the input buffer of the respective node.
    inputCallback();
    
    if (isTransactionActive_)
    {
        // If a transaction is active, don't propagate immediately.
        // Instead, record the node and wait for more inputs.
        changedInputs_.push_back(nodeId);
    }
    else
    {
        // Update the node. This applies the input buffer to the node value and checks if it changed.
        if (nodePtr->Update(0) == UpdateResult::changed)
        {
            if (IsBitmaskSet(node.flags, NodeFlags::buffered))
                pendingBufferedNodes_.push_back(nodePtr);

            // Propagate changes through the graph
            ScheduleSuccessors(node);

            if (! scheduledNodes_.IsEmpty())
                Propagate();
        }

        ClearBufferedNodes();
    }
}

template <typename F>
void SingleThreadedGraph::DoTransaction(TransactionFlags flags, F&& transactionCallback)
{
    // Transaction callback may add multiple inputs.
    isTransactionActive_ = true;
    std::forward<F>(transactionCallback)();
    isTransactionActive_ = false;

    // Apply all buffered inputs.
    for (NodeId nodeId : changedInputs_)
    {
        auto& node = nodeData_[nodeId];
        auto* nodePtr = node.nodePtr;

        if (nodePtr->Update(0) == UpdateResult::changed)
        {
            if (IsBitmaskSet(node.flags, NodeFlags::buffered))
                pendingBufferedNodes_.push_back(nodePtr);

            ScheduleSuccessors(node);
        }
    }

    changedInputs_.clear();

    // Propagate changes through the graph.
    if (! scheduledNodes_.IsEmpty())
        Propagate();

    ClearBufferedNodes();
}

void SingleThreadedGraph::Propagate()
{
    while (scheduledNodes_.FetchNext())
    {
        for (NodeId nodeId : scheduledNodes_.Next())
        {
            auto& node = nodeData_[nodeId];
            auto* nodePtr = node.nodePtr;

            if (node.level < node.newLevel)
            {
                // Re-schedule this node
                node.level = node.newLevel;
                InvalidateSuccessors(node);
                scheduledNodes_.Push(nodeId, node.level);
                continue;
            }

            auto result = nodePtr->Update(0);

            if (result == UpdateResult::changed)
            {
                if (IsBitmaskSet(node.flags, NodeFlags::buffered))
                    pendingBufferedNodes_.push_back(nodePtr);

                ScheduleSuccessors(node);
            }
            else if (result == UpdateResult::shifted)
            {
                // Re-schedule this node
                InvalidateSuccessors(node);
                scheduledNodes_.Push(nodeId, node.level);
                continue;
            }

            node.queued = false;
        }
    }
}

void SingleThreadedGraph::ScheduleSuccessors(NodeData& node)
{
    for (NodeId succId : node.successors)
    {
        auto& succ = nodeData_[succId];

        if (!succ.queued)
        {
            succ.queued = true;
            scheduledNodes_.Push(succId, succ.level);
        }
    }
}

void SingleThreadedGraph::InvalidateSuccessors(NodeData& node)
{
    for (NodeId succId : node.successors)
    {
        auto& succ = nodeData_[succId];

        if (succ.newLevel <= node.level)
            succ.newLevel = node.level + 1;
    }
}

void SingleThreadedGraph::ClearBufferedNodes()
{
    for (IReactiveNode* nodePtr : pendingBufferedNodes_)
        nodePtr->ClearBuffer();
    pendingBufferedNodes_.clear();
}

bool SingleThreadedGraph::TopoQueue::FetchNext()
{
    // Throw away previous values
    nextData_.clear();

    // Find min level of nodes in queue data
    minLevel_ = (std::numeric_limits<int>::max)();
    for (const auto& e : queueData_)
        if (minLevel_ > e.second)
            minLevel_ = e.second;

    // Swap entries with min level to the end
    auto p = std::partition(queueData_.begin(), queueData_.end(), [t = minLevel_] (const Entry& e) { return t != e.second; });

    // Move min level values to next data
    nextData_.reserve(std::distance(p, queueData_.end()));

    for (auto it = p; it != queueData_.end(); ++it)
        nextData_.push_back(it->first);

    // Truncate moved entries
    queueData_.resize(std::distance(queueData_.begin(), p));

    return !nextData_.empty();
}

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_GRAPH_PROPAGATIONST_H_INCLUDED