
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "react/engine/ToposortEngine.h"
#include "tbb/parallel_for.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/
namespace toposort {

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SeqTurn
///////////////////////////////////////////////////////////////////////////////////////////////////
SeqTurn::SeqTurn(TurnIdT id, TransactionFlagsT flags) :
    TurnBase( id, flags )
{}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ParTurn
///////////////////////////////////////////////////////////////////////////////////////////////////
ParTurn::ParTurn(TurnIdT id, TransactionFlagsT flags) :
    TurnBase( id, flags )
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
template class EngineBase<SeqNode,SeqTurn>;
template class EngineBase<ParNode,ParTurn>;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SeqEngineBase
///////////////////////////////////////////////////////////////////////////////////////////////////
void SeqEngineBase::Propagate(SeqTurn& turn)
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

void SeqEngineBase::OnDynamicNodeAttach(SeqNode& node, SeqNode& parent, SeqTurn& turn)
{
    this->OnNodeAttach(node, parent);
    
    invalidateSuccessors(node);

    // Re-schedule this node
    node.Queued = true;
    scheduledNodes_.Push(&node);
}

void SeqEngineBase::OnDynamicNodeDetach(SeqNode& node, SeqNode& parent, SeqTurn& turn)
{
    this->OnNodeDetach(node, parent);
}

void SeqEngineBase::processChildren(SeqNode& node, SeqTurn& turn)
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

void SeqEngineBase::invalidateSuccessors(SeqNode& node)
{
    for (auto* succ : node.Successors)
    {
        if (succ->NewLevel <= node.Level)
            succ->NewLevel = node.Level + 1;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ParEngineBase
///////////////////////////////////////////////////////////////////////////////////////////////////
void ParEngineBase::Propagate(ParTurn& turn)
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

void ParEngineBase::OnDynamicNodeAttach(ParNode& node, ParNode& parent, ParTurn& turn)
{
    DynRequestData data{ true, &node, &parent };
    dynRequests_.push_back(data);
}

void ParEngineBase::OnDynamicNodeDetach(ParNode& node, ParNode& parent, ParTurn& turn)
{
    DynRequestData data{ false, &node, &parent };
    dynRequests_.push_back(data);
}

void ParEngineBase::applyDynamicAttach(ParNode& node, ParNode& parent, ParTurn& turn)
{
    this->OnNodeAttach(node, parent);

    invalidateSuccessors(node);

    // Re-schedule this node
    node.Collected = true;
    topoQueue_.Push(&node);
}

void ParEngineBase::applyDynamicDetach(ParNode& node, ParNode& parent, ParTurn& turn)
{
    this->OnNodeDetach(node, parent);
}

void ParEngineBase::processChildren(ParNode& node, ParTurn& turn)
{
    // Add children to queue
    for (auto* succ : node.Successors)
        if (!succ->Collected.exchange(true, std::memory_order_relaxed))
            topoQueue_.Push(succ);
}

void ParEngineBase::invalidateSuccessors(ParNode& node)
{
    for (auto* succ : node.Successors)
    {// succ->InvalidateMutex
        ParNode::InvalidateMutexT::scoped_lock lock(succ->InvalidateMutex);

        if (succ->NewLevel <= node.Level)
            succ->NewLevel = node.Level + 1;
    }// ~succ->InvalidateMutex
}

} // ~namespace toposort
/****************************************/ REACT_IMPL_END /***************************************/