
//          Copyright Sebastian Jeckel 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_GRAPH_PROPAGATIONST_H_INCLUDED
#define REACT_DETAIL_GRAPH_PROPAGATIONST_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <algorithm>
#include <atomic>
#include <type_traits>
#include <utility>
#include <vector>
#include <map>
#include <mutex>

#include <tbb/concurrent_queue.h>
#include <tbb/task.h>

#include "react/common/Containers.h"
#include "react/common/Types.h"
#include "react/detail/IReactiveGraph.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

template <typename K>
class PtrCache
{
public:
    template <typename V, typename F>
    std::shared_ptr<V> LookupOrCreate(const K& key, F&& createFunc)
    {
        std::lock_guard<std::mutex> scopedLock(mutex_);

        auto it = map1_.find(key);

        if (it != map1_.end())
        {
            if (auto ptr = it->second.lock())
            {
                return std::static_pointer_cast<V>(ptr);
            }
        }

        std::shared_ptr<V> v = createFunc();
        auto res = map1_.insert({ key, std::weak_ptr<void>{ v } });
        map2_[v.get()] = res.first;
        return v;
    }

    template <typename V>
    void Erase(V* ptr)
    {
        std::lock_guard<std::mutex> scopedLock(mutex_);

        auto it = map2_.find((void*)ptr);

        if (it != map2_.end())
        {
            map1_.erase(it->second);
            map2_.erase(it);
        }
    }

private:
    std::mutex  mutex_;

    using Map1Type = std::map<K, std::weak_ptr<void>>;
    using Map2Type = std::unordered_map<void*, typename Map1Type::iterator>;

    Map1Type map1_;
    Map2Type map2_;
};

class ReactiveGraph;

class TransactionQueue
{
public:
    TransactionQueue(ReactiveGraph& graph) :
        graph_( graph )
        { }

    TransactionQueue(const TransactionQueue&) = delete;
    TransactionQueue& operator=(const TransactionQueue&) = delete;

    TransactionQueue(TransactionQueue&&) = default;
    TransactionQueue& operator =(TransactionQueue&&) = default;

    template <typename F>
    void Push(TransactionFlags flags, F&& transaction)
    {
        transactions_.push(StoredTransaction{ flags, std::forward<F>(transaction) });

        if (count_.fetch_add(1, std::memory_order_relaxed) == 0)
            tbb::task::enqueue(*new(tbb::task::allocate_root()) WorkerTask(*this));
    }

private:
    struct StoredTransaction
    {
        TransactionFlags        flags;
        std::function<void()>   callback;
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

    ReactiveGraph& graph_;
};

class ReactiveGraph
{
public:
    using LinkCacheType = PtrCache<std::pair<void*, void*>>;

    NodeId RegisterNode(IReactiveNode* nodePtr, NodeCategory category);
    void UnregisterNode(NodeId nodeId);

    void OnNodeAttach(NodeId node, NodeId parentId);
    void OnNodeDetach(NodeId node, NodeId parentId);

    template <typename F>
    void AddInput(NodeId nodeId, F&& inputCallback);

    template <typename F>
    void DoTransaction(F&& transactionCallback);

    template <typename F>
    void EnqueueTransaction(TransactionFlags flags, F&& transactionCallback);
    
    LinkCacheType& GetLinkCache()
        { return linkCache_; }

private:
    struct NodeData
    {
        NodeData() = default;

        NodeData(const NodeData&) = default;
        NodeData& operator=(const NodeData&) = default;

        NodeData(IReactiveNode* nodePtrIn, NodeCategory categoryIn) :
            nodePtr( nodePtrIn ),
            category(categoryIn)
            { }

        NodeCategory category = NodeCategory::normal;

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
    void UpdateLinkNodes();

    void ScheduleSuccessors(NodeData & node);
    void InvalidateSuccessors(NodeData & node);

private:
    TransactionQueue    transactionQueue_{ *this };

    IndexedStorage<NodeData>    nodeData_;

    TopoQueue           scheduledNodes_;
    std::vector<NodeId> changedInputs_;
    LinkOutputMap       scheduledLinkOutputs_;

    LinkCacheType       linkCache_;

    bool isTransactionActive_ = false;
};

NodeId ReactiveGraph::RegisterNode(IReactiveNode* nodePtr, NodeCategory category)
{
    return nodeData_.Insert(NodeData{ nodePtr, category });
}

void ReactiveGraph::UnregisterNode(NodeId nodeId)
{
    nodeData_.Erase(nodeId);
}

void ReactiveGraph::OnNodeAttach(NodeId nodeId, NodeId parentId)
{
    auto& node = nodeData_[nodeId];
    auto& parent = nodeData_[parentId];

    parent.successors.push_back(nodeId);

    if (node.level <= parent.level)
        node.level = parent.level + 1;
}

void ReactiveGraph::OnNodeDetach(NodeId nodeId, NodeId parentId)
{
    auto& parent = nodeData_[parentId];
    auto& successors = parent.successors;

    successors.erase(std::find(successors.begin(), successors.end(), nodeId));
}

template <typename F>
void ReactiveGraph::AddInput(NodeId nodeId, F&& inputCallback)
{
    auto& node = nodeData_[nodeId];
    auto* nodePtr = node.nodePtr;

    // This writes to the input buffer of the respective node.
    inputCallback();
    
    if (isTransactionActive_)
    {
        // If a transaction is active, don't propagate immediately.
        // Instead, record the node and wait for more inputs.
        changedInputs_.push_back(nodeId);
    }
    else
    {
        size_t successorCount = node.successors.size();

        // Update the node. This applies the input buffer to the node value and checks if it changed.
        if (nodePtr->Update(0, successorCount) == UpdateResult::changed)
        {
            // Propagate changes through the graph
            ScheduleSuccessors(node);

            if (! scheduledNodes_.IsEmpty())
                Propagate();

            if (! scheduledLinkOutputs_.empty())
                UpdateLinkNodes();
        }
    }
}

template <typename F>
void ReactiveGraph::DoTransaction(F&& transactionCallback)
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

        size_t successorCount = node.successors.size();

        if (nodePtr->Update(0, successorCount) == UpdateResult::changed)
        {
            if (node.category == NodeCategory::dyninput)
                InvalidateSuccessors(node);

            ScheduleSuccessors(node);
        }
    }

    changedInputs_.clear();

    // Propagate changes through the graph.
    if (! scheduledNodes_.IsEmpty())
        Propagate();

    if (! scheduledLinkOutputs_.empty())
        UpdateLinkNodes();
}

template <typename F>
void ReactiveGraph::EnqueueTransaction(TransactionFlags flags, F&& transactionCallback)
{
    transactionQueue_.Push(flags, std::forward<F>(transactionCallback));
}

void ReactiveGraph::Propagate()
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

            // Special handling for link output nodes. They have no successors and they don't have to be updated.
            if (node.category == NodeCategory::linkoutput)
            {
                static_cast<ILinkOutputNode*>(node.nodePtr)->CollectOutput(scheduledLinkOutputs_);
                continue;
            }

            size_t successorCount = node.successors.size();

            if (nodePtr->Update(0u, successorCount) == UpdateResult::changed)
            {
                ScheduleSuccessors(node);
            }

            node.queued = false;
        }
    }
}

void ReactiveGraph::UpdateLinkNodes()
{
    for (auto& e : scheduledLinkOutputs_)
    {
        e.first->EnqueueTransaction(TransactionFlags::none,
            [inputs = std::move(e.second)]
            {
                for (auto& callback : inputs)
                    callback();
            });
    }

    scheduledLinkOutputs_.clear();
}

void ReactiveGraph::ScheduleSuccessors(NodeData& node)
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

void ReactiveGraph::InvalidateSuccessors(NodeData& node)
{
    for (NodeId succId : node.successors)
    {
        auto& succ = nodeData_[succId];

        if (succ.newLevel <= node.level)
            succ.newLevel = node.level + 1;
    }
}

bool ReactiveGraph::TopoQueue::FetchNext()
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

void TransactionQueue::ProcessQueue()
{
    for (;;)
    {
        size_t popCount = ProcessNextBatch();
        if (count_.fetch_sub(popCount) == popCount)
            return;
    }
}

size_t TransactionQueue::ProcessNextBatch()
{
    StoredTransaction curTransaction;
    size_t popCount = 0;
    bool canMerge = false;
    bool skipPop = false;
    bool isDone = false;

    // Outer loop. One transaction per iteration.
    for (;;)
    {
        if (!skipPop)
        {
            if (!transactions_.try_pop(curTransaction))
                return popCount;

            canMerge = IsBitmaskSet(curTransaction.flags, TransactionFlags::allow_merging);
            ++popCount;
        }
        else
        {
            skipPop = false;
        }

        graph_.DoTransaction([&]
        {
            curTransaction.callback();

            if (canMerge)
            {
                // Inner loop. Mergeable transactions are merged
                for (;;)
                {
                    if (transactions_.try_pop(curTransaction))
                        return;

                    canMerge = IsBitmaskSet(curTransaction.flags, TransactionFlags::allow_merging);
                    ++popCount;

                    if (!canMerge)
                    {
                        skipPop = true;
                        return;
                    }

                    curTransaction.callback();
                }
            }
        });
    }
}

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_GRAPH_PROPAGATIONST_H_INCLUDED