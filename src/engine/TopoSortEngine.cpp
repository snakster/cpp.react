
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "react/engine/TopoSortEngine.h"
#include "tbb/parallel_for.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/
namespace toposort {

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Turn
///////////////////////////////////////////////////////////////////////////////////////////////////
ExclusiveTurn::ExclusiveTurn(TurnIdT id, TurnFlagsT flags) :
    TurnBase(id, flags)
{
}

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
void EngineBase<TNode,TTurn>::OnTurnInputChange(TNode& node, TTurn& turn)
{
    processChildren(node, turn);
}

template <typename TNode, typename TTurn>
void EngineBase<TNode,TTurn>::OnNodePulse(TNode& node, TTurn& turn)
{
    processChildren(node, turn);
}

// Explicit instantiation
template class EngineBase<SeqNode,ExclusiveTurn>;
template class EngineBase<ParNode,ExclusiveTurn>;
template class EngineBase<SeqNode,DefaultQueueableTurn<ExclusiveTurn>>;
template class EngineBase<ParNode,DefaultQueueableTurn<ExclusiveTurn>>;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SeqEngineBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurn>
void SeqEngineBase<TTurn>::OnTurnPropagate(TTurn& turn)
{
    while (scheduledNodes_.FetchNext())
    {
        for (auto* curNode : scheduledNodes_.NextNodes())
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
    OnNodeAttach(node, parent);
    
    invalidateSuccessors(node);

    // Re-schedule this node
    node.Queued = true;
    scheduledNodes_.Push(&node);
}

template <typename TTurn>
void SeqEngineBase<TTurn>::OnDynamicNodeDetach(SeqNode& node, SeqNode& parent, TTurn& turn)
{
    OnNodeDetach(node, parent);
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
template class SeqEngineBase<ExclusiveTurn>;
template class SeqEngineBase<DefaultQueueableTurn<ExclusiveTurn>>;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ParEngineBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurn>
void ParEngineBase<TTurn>::OnTurnPropagate(TTurn& turn)
{
    while (topoQueue_.FetchNext())
    {
        //using RangeT = tbb::blocked_range<vector<ParNode*>::const_iterator>;
        using RangeT = ParEngineBase::TopoQueueT::RangeT;

        // Iterate all nodes of current level and start processing them in parallel
        tbb::parallel_for(
            topoQueue_.NextRange(),
            [&] (const RangeT& range)
            {
                for (auto* curNode : range)
                {
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
void ParEngineBase<TTurn>::HintUpdateDuration(ParNode& node, uint dur)
{
    if (dur < min_weight)
        dur = min_weight;

    node.Weight = dur;
}

template <typename TTurn>
void ParEngineBase<TTurn>::applyDynamicAttach(ParNode& node, ParNode& parent, TTurn& turn)
{
    OnNodeAttach(node, parent);

    invalidateSuccessors(node);

    // Re-schedule this node
    node.Collected = true;
    topoQueue_.Push(&node);
}

template <typename TTurn>
void ParEngineBase<TTurn>::applyDynamicDetach(ParNode& node, ParNode& parent, TTurn& turn)
{
    OnNodeDetach(node, parent);
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
template class ParEngineBase<ExclusiveTurn>;
template class ParEngineBase<DefaultQueueableTurn<ExclusiveTurn>>;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// PipeliningTurn
///////////////////////////////////////////////////////////////////////////////////////////////////
PipeliningTurn::PipeliningTurn(TurnIdT id, TurnFlagsT flags) :
    TurnBase(id, flags),
    isMergeable_{ (flags & enable_input_merging) != 0 }
{
}

bool PipeliningTurn::AdvanceLevel()
{
    std::unique_lock<mutex> lock(advMutex_);

    advCondition_.wait(lock, [this] {
        return !(currentLevel_ + 1 > maxLevel_);
    });

    // Remove the intervals we're going to leave
    auto it = levelIntervals_.begin();
    while (it != levelIntervals_.end())
    {
        if (it->second <= currentLevel_)
            it = levelIntervals_.erase(it);
        else
            ++it;
    }

    // Add new interval for current level
    if (currentLevel_ < curUpperBound_)
        levelIntervals_.emplace(currentLevel_, curUpperBound_);

    currentLevel_++;

    curUpperBound_ = currentLevel_;
    
    // Min level is the minimum over all interval lower bounds
    int newMinLevel =
        levelIntervals_.begin() != levelIntervals_.end() ?
            levelIntervals_.begin()->first :
            -1;

    if (minLevel_ != newMinLevel)
    {
        minLevel_ = newMinLevel;
        return true;
    }
    else
    {
        return false;
    }
}

void PipeliningTurn::SetMaxLevel(int level)
{
    std::lock_guard<mutex> lock(advMutex_);

    maxLevel_ = level;
    advCondition_.notify_all();
}

void PipeliningTurn::WaitForMaxLevel(int targetLevel)
{
    std::unique_lock<mutex> lock(advMutex_);

    advCondition_.wait(lock, [this, targetLevel] {
        return !(targetLevel < maxLevel_);
    });
}

void PipeliningTurn::Append(PipeliningTurn* turn)
{
    successor_ = turn;

    if (turn)
        turn->predecessor_ = this;

    UpdateSuccessor();
}

void PipeliningTurn::UpdateSuccessor()
{
    if (successor_)
        successor_->SetMaxLevel(minLevel_ - 1);
}

void PipeliningTurn::Remove()
{
    if (predecessor_)
    {
        predecessor_->Append(successor_);
    }
    else if (successor_)
    {
        successor_->SetMaxLevel(INT_MAX);
        successor_->predecessor_ = nullptr;
    }

    for (const auto& e : merged_)
        e.second->Unblock();
}

void PipeliningTurn::AdjustUpperBound(int level)
{
    if (curUpperBound_ < level)
        curUpperBound_ = level;
}

void PipeliningTurn::RunMergedInputs() const
{
    for (const auto& e : merged_)
        e.first();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// PipeliningEngine
///////////////////////////////////////////////////////////////////////////////////////////////////
void PipeliningEngine::OnNodeAttach(ParNode& node, ParNode& parent)
{
    parent.Successors.Add(node);

    if (node.Level <= parent.Level)
        node.Level = parent.Level + 1;

    if (node.IsDynamicNode())
    {
        dynamicNodes_.insert(&node);

        if (maxDynamicLevel_ < node.Level)
            maxDynamicLevel_ = node.Level;
    }
}

void PipeliningEngine::OnNodeDetach(ParNode& node, ParNode& parent)
{
    parent.Successors.Remove(node);

    // Recalc maxdynamiclevel?
    if (node.IsDynamicNode())
    {
        dynamicNodes_.erase(&node);
        if (maxDynamicLevel_ == node.Level)
        {
            maxDynamicLevel_ = 0;
            for (auto e : dynamicNodes_)
                if (maxDynamicLevel_ < e->Level)
                    maxDynamicLevel_ = e->Level;
        }
    }
}

void PipeliningEngine::OnTurnAdmissionStart(PipeliningTurn& turn)
{
    {// seqMutex_
        SeqMutexT::scoped_lock lock(seqMutex_);

        if (tail_)
            tail_->Append(&turn);

        tail_ = &turn;
    }// ~seqMutex_

    advanceTurn(turn);
}

void PipeliningEngine::OnTurnAdmissionEnd(PipeliningTurn& turn)
{
    turn.RunMergedInputs();
}

void PipeliningEngine::OnTurnEnd(PipeliningTurn& turn)
{// seqMutex_ (write)
    SeqMutexT::scoped_lock    lock(seqMutex_, true);

    turn.Remove();

    if (tail_ == &turn)
        tail_ = nullptr;
}// ~seqMutex_

void PipeliningEngine::OnTurnInputChange(ParNode& node, PipeliningTurn& turn)
{
    processChildren(node, turn);
}

void PipeliningEngine::OnNodePulse(ParNode& node, PipeliningTurn& turn)
{
    processChildren(node, turn);
}

void PipeliningEngine::OnTurnPropagate(PipeliningTurn& turn)
{
    if (maxDynamicLevel_ > 0)
        turn.AdjustUpperBound(maxDynamicLevel_);

    while (turn.TopoQueue.FetchNext())
    {
        using RangeT = PipeliningTurn::TopoQueueT::RangeT;

        for (const auto* node : turn.TopoQueue.NextRange())
            turn.AdjustUpperBound(node->Level);

        advanceTurn(turn);

        // Iterate all nodes of current level and start processing them in parallel
        tbb::parallel_for(
            turn.TopoQueue.NextRange(),
            [&] (const RangeT& range)
            {
                for (auto* curNode : range)
                {
                    if (curNode->Level < curNode->NewLevel)
                    {
                        curNode->Level = curNode->NewLevel;
                        invalidateSuccessors(*curNode);
                        turn.TopoQueue.Push(curNode);
                        continue;
                    }

                    curNode->Collected = false;

                    // Tick -> if changed: OnNodePulse -> adds child nodes to the queue
                    curNode->Tick(&turn);
                }
            }
        );

        if (turn.DynRequests.size() > 0)
        {
            for (auto req : turn.DynRequests)
            {
                if (req.ShouldAttach)
                    applyDynamicAttach(*req.Node, *req.Parent, turn);
                else
                    applyDynamicDetach(*req.Node, *req.Parent, turn);
            }
            turn.DynRequests.clear();
        }
    }
}

void PipeliningEngine::OnDynamicNodeAttach(ParNode& node, ParNode& parent, PipeliningTurn& turn)
{
    DynRequestData data{ true, &node, &parent };
    turn.DynRequests.push_back(data);
}

void PipeliningEngine::OnDynamicNodeDetach(ParNode& node, ParNode& parent, PipeliningTurn& turn)
{
    DynRequestData data{ false, &node, &parent };
    turn.DynRequests.push_back(data);
}

void PipeliningEngine::applyDynamicAttach(ParNode& node, ParNode& parent, PipeliningTurn& turn)
{
    turn.WaitForMaxLevel(INT_MAX);

    OnNodeAttach(node, parent);
    
    invalidateSuccessors(node);

    // Re-schedule this node
    node.Collected = true;
    turn.TopoQueue.Push(&node);    
}

void PipeliningEngine::applyDynamicDetach(ParNode& node, ParNode& parent, PipeliningTurn& turn)
{
    OnNodeDetach(node, parent);
}

void PipeliningEngine::processChildren(ParNode& node, PipeliningTurn& turn)
{
    // Add children to queue
    for (auto* succ : node.Successors)
        if (!succ->Collected.exchange(true, std::memory_order_relaxed))
            turn.TopoQueue.Push(succ);
}

void PipeliningEngine::invalidateSuccessors(ParNode& node)
{
    for (auto* succ : node.Successors)
    {// succ->InvalidateMutex
        ParNode::InvalidateMutexT::scoped_lock lock(succ->InvalidateMutex);

        if (succ->NewLevel <= node.Level)
        {
            auto newLevel = node.Level + 1;
            succ->NewLevel = newLevel;

            {// maxDynamicLevelMutex_
                MaxDynamicLevelMutexT::scoped_lock lock(maxDynamicLevelMutex_);
                if (succ->IsDynamicNode() && maxDynamicLevel_ < newLevel)
                    maxDynamicLevel_ = newLevel;
            }// ~maxDynamicLevelMutex_
        }
    }// ~succ->InvalidateMutex
}

void PipeliningEngine::advanceTurn(PipeliningTurn& turn)
{
    // No need to wake up successor if min level did not change
    if (turn.AdvanceLevel())
        return;    

    {// seqMutex_ (read)
        SeqMutexT::scoped_lock    lock(seqMutex_, false);

        turn.UpdateSuccessor();
    }// ~seqMutex_
}

} // ~namespace toposort
/****************************************/ REACT_IMPL_END /***************************************/