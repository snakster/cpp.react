
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_GRAPH_IMPL_H_INCLUDED
#define REACT_DETAIL_GRAPH_IMPL_H_INCLUDED

#pragma once

#include "react/detail/defs.h"

#include <algorithm>
#include <atomic>
#include <type_traits>
#include <utility>
#include <vector>
#include <map>
#include <mutex>

#include <tbb/concurrent_queue.h>
#include <tbb/task.h>

#include "react/common/ptrcache.h"
#include "react/common/slotmap.h"
#include "react/common/syncpoint.h"
#include "react/detail/graph_interface.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

class ReactGraph;

class TransactionQueue
{
public:
    TransactionQueue(ReactGraph& graph) :
        graph_( graph )
    { }

    template <typename F>
    void Push(F&& func, SyncPoint::Dependency dep, TransactionFlags flags)
    {
        transactions_.push(StoredTransaction{ std::forward<F>(func), std::move(dep), flags });

        if (count_.fetch_add(1, std::memory_order_release) == 0)
            tbb::task::enqueue(*new(tbb::task::allocate_root()) WorkerTask(*this));
    }

private:
    struct StoredTransaction
    {
        std::function<void()>   func;
        SyncPoint::Dependency   dep;
        TransactionFlags        flags;
    };

    class WorkerTask : public tbb::task
    {
    public:
        WorkerTask(TransactionQueue& parent) :
            parent_( parent )
        { }

        tbb::task* execute()
        {
            parent_.ProcessQueue();
            return nullptr;
        }

    private:
        TransactionQueue& parent_;
    };

    void ProcessQueue();

    size_t ProcessNextBatch();

    tbb::concurrent_queue<StoredTransaction> transactions_;

    std::atomic<size_t> count_{ 0 };

    ReactGraph& graph_;
};

class ReactGraph
{
public:
    using LinkCache = WeakPtrCache<void*, IReactNode>;

    NodeId RegisterNode(IReactNode* nodePtr, NodeCategory category);
    void UnregisterNode(NodeId nodeId);

    void AttachNode(NodeId node, NodeId parentId);
    void DetachNode(NodeId node, NodeId parentId);

    template <typename F>
    void PushInput(NodeId nodeId, F&& inputCallback);

    void AddSyncPointDependency(SyncPoint::Dependency dep, bool syncLinked);

    void AllowLinkedTransactionMerging(bool allowMerging);

    template <typename F>
    void DoTransaction(F&& transactionCallback);

    template <typename F>
    void EnqueueTransaction(F&& func, SyncPoint::Dependency dep, TransactionFlags flags);
    
    LinkCache& GetLinkCache()
        { return linkCache_; }

private:
    struct NodeData
    {
        NodeData() = default;

        NodeData(const NodeData&) = default;
        NodeData& operator=(const NodeData&) = default;

        NodeData(IReactNode* nodePtrIn, NodeCategory categoryIn) :
            nodePtr( nodePtrIn ),
            category(categoryIn)
        { }

        NodeCategory category = NodeCategory::normal;

        int     level       = 0;
        int     newLevel    = 0 ;
        bool    queued      = false;

        IReactNode*  nodePtr = nullptr;

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
    void UpdateLinkNodes();

    void ScheduleSuccessors(NodeData & node);
    void RecalculateSuccessorLevels(NodeData & node);

private:
    TransactionQueue    transactionQueue_{ *this };

    SlotMap<NodeData>   nodeData_;

    TopoQueue scheduledNodes_;

    std::vector<NodeId>         changedInputs_;
    std::vector<IReactNode*>    changedNodes_;

    LinkOutputMap scheduledLinkOutputs_;

    std::vector<SyncPoint::Dependency> localDependencies_;
    std::vector<SyncPoint::Dependency> linkDependencies_;

    LinkCache linkCache_;

    int  transactionLevel_ = 0;
    bool allowLinkedTransactionMerging_ = false;
};

template <typename F>
void ReactGraph::PushInput(NodeId nodeId, F&& inputCallback)
{
    auto& node = nodeData_[nodeId];
    auto* nodePtr = node.nodePtr;

    // This writes to the input buffer of the respective node.
    std::forward<F>(inputCallback)();
    
    changedInputs_.push_back(nodeId);

    if (transactionLevel_ == 0)
        Propagate();
}

template <typename F>
void ReactGraph::DoTransaction(F&& transactionCallback)
{
    // Transaction callback may add multiple inputs.
    ++transactionLevel_;
    std::forward<F>(transactionCallback)();
    --transactionLevel_;

    Propagate();
}

template <typename F>
void ReactGraph::EnqueueTransaction(F&& func, SyncPoint::Dependency dep, TransactionFlags flags)
{
    transactionQueue_.Push(std::forward<F>(func), std::move(dep), flags);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// GroupInternals
///////////////////////////////////////////////////////////////////////////////////////////////////
class GroupInternals
{
public:
    GroupInternals() :
        graphPtr_( std::make_shared<ReactGraph>() )
    {  }

    GroupInternals(const GroupInternals&) = default;
    GroupInternals& operator=(const GroupInternals&) = default;

    GroupInternals(GroupInternals&&) = default;
    GroupInternals& operator=(GroupInternals&&) = default;

    auto GetGraphPtr() -> std::shared_ptr<ReactGraph>&
        { return graphPtr_; }

    auto GetGraphPtr() const -> const std::shared_ptr<ReactGraph>&
        { return graphPtr_; }

private:
    std::shared_ptr<ReactGraph> graphPtr_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_GRAPH_IMPL_H_INCLUDED