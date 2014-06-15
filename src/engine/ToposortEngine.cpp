
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "react/engine/ToposortEngine.h"
#include "tbb/parallel_for.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/
namespace toposort {

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ExclusiveSeqTurn
///////////////////////////////////////////////////////////////////////////////////////////////////
ExclusiveSeqTurn::ExclusiveSeqTurn(TurnIdT id, TurnFlagsT flags) :
    TurnBase(id, flags)
{}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ExclusiveParTurn
///////////////////////////////////////////////////////////////////////////////////////////////////
ExclusiveParTurn::ExclusiveParTurn(TurnIdT id, TurnFlagsT flags) :
    TurnBase(id, flags)
{}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EngineBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TNode, typename TTurn>
void EngineBase<TNode,TTurn>::OnNodeAttach(TNode& node, TNode& parent)
{
    parent.Successors.Add(node);

    if (node.Level <= parent.Level)
        node.Level = parent.Level + 1;
}

template <typename TNode, typename TTurn>
void EngineBase<TNode,TTurn>::OnNodeDetach(TNode& node, TNode& parent)
{
    parent.Successors.Remove(node);
}

template <typename TNode, typename TTurn>
void EngineBase<TNode,TTurn>::OnInputChange(TNode& node, TTurn& turn)
{
    processChildren(node, turn);
}

template <typename TNode, typename TTurn>
void EngineBase<TNode,TTurn>::OnNodePulse(TNode& node, TTurn& turn)
{
    processChildren(node, turn);
}

// Explicit instantiation
template class EngineBase<SeqNode,ExclusiveSeqTurn>;
template class EngineBase<ParNode,ExclusiveParTurn>;
template class EngineBase<SeqNode,DefaultQueueableTurn<ExclusiveSeqTurn>>;
template class EngineBase<ParNode,DefaultQueueableTurn<ExclusiveParTurn>>;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SeqEngineBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurn>
void SeqEngineBase<TTurn>::Propagate(TTurn& turn)
{
    while (scheduledNodes_.FetchNext())
    {
        for (auto* curNode : scheduledNodes_.NextValues())
        {
            if (curNode->Level < curNode->NewLevel)
            {
                curNode->Level = curNode->NewLevel;
                invalidateSuccessors(*curNode);
                scheduledNodes_.Push(curNode);
                continue;
            }

            curNode->Queued = false;
            curNode->Tick(&turn);
        }
    }
}

template <typename TTurn>
void SeqEngineBase<TTurn>::OnDynamicNodeAttach(SeqNode& node, SeqNode& parent, TTurn& turn)
{
    this->OnNodeAttach(node, parent);
    
    invalidateSuccessors(node);

    // Re-schedule this node
    node.Queued = true;
    scheduledNodes_.Push(&node);
}

template <typename TTurn>
void SeqEngineBase<TTurn>::OnDynamicNodeDetach(SeqNode& node, SeqNode& parent, TTurn& turn)
{
    this->OnNodeDetach(node, parent);
}

template <typename TTurn>
void SeqEngineBase<TTurn>::processChildren(SeqNode& node, TTurn& turn)
{
    // Add children to queue
    for (auto* succ : node.Successors)
    {
        if (!succ->Queued)
        {
            succ->Queued = true;
            scheduledNodes_.Push(succ);
        }
    }
}

template <typename TTurn>
void SeqEngineBase<TTurn>::invalidateSuccessors(SeqNode& node)
{
    for (auto* succ : node.Successors)
    {
        if (succ->NewLevel <= node.Level)
            succ->NewLevel = node.Level + 1;
    }
}

// Explicit instantiation
template class SeqEngineBase<ExclusiveSeqTurn>;
template class SeqEngineBase<DefaultQueueableTurn<ExclusiveSeqTurn>>;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ParEngineBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurn>
void ParEngineBase<TTurn>::Propagate(TTurn& turn)
{
    while (topoQueue_.FetchNext())
    {
        //using RangeT = tbb::blocked_range<vector<ParNode*>::const_iterator>;
        using RangeT = ParEngineBase::TopoQueueT::NextRangeT;

        // Iterate all nodes of current level and start processing them in parallel
        tbb::parallel_for(
            topoQueue_.NextRange(),
            [&] (const RangeT& range)
            {
                for (const auto& e : range)
                {
                    auto* curNode = e.first;

                    if (curNode->Level < curNode->NewLevel)
                    {
                        curNode->Level = curNode->NewLevel;
                        invalidateSuccessors(*curNode);
                        topoQueue_.Push(curNode);
                        continue;
                    }

                    curNode->Collected = false;

                    // Tick -> if changed: OnNodePulse -> adds child nodes to the queue
                    curNode->Tick(&turn);
                }
            }
        );

        if (dynRequests_.size() > 0)
        {
            for (auto req : dynRequests_)
            {
                if (req.ShouldAttach)
                    applyDynamicAttach(*req.Node, *req.Parent, turn);
                else
                    applyDynamicDetach(*req.Node, *req.Parent, turn);
            }
            dynRequests_.clear();
        }
    }
}

template <typename TTurn>
void ParEngineBase<TTurn>::OnDynamicNodeAttach(ParNode& node, ParNode& parent, TTurn& turn)
{
    DynRequestData data{ true, &node, &parent };
    dynRequests_.push_back(data);
}

template <typename TTurn>
void ParEngineBase<TTurn>::OnDynamicNodeDetach(ParNode& node, ParNode& parent, TTurn& turn)
{
    DynRequestData data{ false, &node, &parent };
    dynRequests_.push_back(data);
}

template <typename TTurn>
void ParEngineBase<TTurn>::applyDynamicAttach(ParNode& node, ParNode& parent, TTurn& turn)
{
    this->OnNodeAttach(node, parent);

    invalidateSuccessors(node);

    // Re-schedule this node
    node.Collected = true;
    topoQueue_.Push(&node);
}

template <typename TTurn>
void ParEngineBase<TTurn>::applyDynamicDetach(ParNode& node, ParNode& parent, TTurn& turn)
{
    this->OnNodeDetach(node, parent);
}

template <typename TTurn>
void ParEngineBase<TTurn>::processChildren(ParNode& node, TTurn& turn)
{
    // Add children to queue
    for (auto* succ : node.Successors)
        if (!succ->Collected.exchange(true, std::memory_order_relaxed))
            topoQueue_.Push(succ);
}

template <typename TTurn>
void ParEngineBase<TTurn>::invalidateSuccessors(ParNode& node)
{
    for (auto* succ : node.Successors)
    {// succ->InvalidateMutex
        ParNode::InvalidateMutexT::scoped_lock lock(succ->InvalidateMutex);

        if (succ->NewLevel <= node.Level)
            succ->NewLevel = node.Level + 1;
    }// ~succ->InvalidateMutex
}

// Explicit instantiation
template class ParEngineBase<ExclusiveParTurn>;
template class ParEngineBase<DefaultQueueableTurn<ExclusiveParTurn>>;

} // ~namespace toposort
/****************************************/ REACT_IMPL_END /***************************************/