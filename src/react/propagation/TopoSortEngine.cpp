
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "react/propagation/TopoSortEngine.h"

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

template <typename TNode, typename TTurn>
void EngineBase<TNode,TTurn>::invalidateSuccessors(TNode& node)
{
    for (auto* succ : node.Successors)
    {
        if (succ->NewLevel <= node.Level)
            succ->NewLevel = node.Level + 1;
    }
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
    while (!scheduledNodes_.Empty())
    {
        auto node = scheduledNodes_.Top();
        scheduledNodes_.Pop();

        if (node->Level < node->NewLevel)
        {
            node->Level = node->NewLevel;
            invalidateSuccessors(*node);
            scheduledNodes_.Push(node);
            continue;
        }

        node->Queued = false;
        node->Tick(&turn);
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

// Explicit instantiation
template class SeqEngineBase<ExclusiveTurn>;
template class SeqEngineBase<DefaultQueueableTurn<ExclusiveTurn>>;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ParEngineBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurn>
void ParEngineBase<TTurn>::OnTurnPropagate(TTurn& turn)
{
    while (!collectBuffer_.empty() || !scheduledNodes_.Empty())
    {
        // Merge thread-safe buffer of nodes that pulsed during last turn into queue
        for (auto node : collectBuffer_)
            scheduledNodes_.Push(node);
        collectBuffer_.clear();

        auto curNode = scheduledNodes_.Top();
        auto currentLevel = curNode->Level;

        // Pop all nodes of current level and start processing them in parallel
        do
        {
            scheduledNodes_.Pop();

            if (curNode->Level < curNode->NewLevel)
            {
                curNode->Level = curNode->NewLevel;
                invalidateSuccessors(*curNode);
                scheduledNodes_.Push(curNode);
                break;
            }

            curNode->Collected = false;

            // Tick -> if changed: OnNodePulse -> adds child nodes to the queue
            tasks_.run(std::bind(&ParNode::Tick, curNode, &turn));

            if (scheduledNodes_.Empty())
                break;

            curNode = scheduledNodes_.Top();
        }
        while (curNode->Level == currentLevel);

        // Wait for tasks of current level
        tasks_.wait();

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

//tbb::parallel_for(tbb::blocked_range<NodeVectorT::iterator>(curNodes.begin(), curNodes.end(), 1),
//    [&] (const tbb::blocked_range<NodeVectorT::iterator>& range)
//    {
//        for (const auto& succ : range)
//        {
//            succ->Counter++;

//            if (succ->SetMark(mark))
//                nextNodes.push_back(succ);
//        }
//    }
//);

template <typename TTurn>
void ParEngineBase<TTurn>::applyDynamicAttach(ParNode& node, ParNode& parent, TTurn& turn)
{
    OnNodeAttach(node, parent);

    invalidateSuccessors(node);

    // Re-schedule this node
    node.Collected = true;
    collectBuffer_.push_back(&node);
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
    {
        if (!succ->Collected.exchange(true))
            collectBuffer_.push_back(succ);
    }
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

    while (!turn.CollectBuffer.empty() || !turn.ScheduledNodes.Empty())
    {
        // Merge thread-safe buffer of nodes that pulsed during last turn into queue
        for (auto node : turn.CollectBuffer)
        {
            turn.AdjustUpperBound(node->Level);
            turn.ScheduledNodes.Push(node);
        }
        turn.CollectBuffer.clear();

        advanceTurn(turn);

        auto curNode = turn.ScheduledNodes.Top();
        auto currentLevel = curNode->Level;

        // Pop all nodes of current level and start processing them in parallel
        do
        {
            turn.ScheduledNodes.Pop();

            if (curNode->Level < curNode->NewLevel)
            {
                curNode->Level = curNode->NewLevel;
                invalidateSuccessors(*curNode);
                turn.ScheduledNodes.Push(curNode);
                break;
            }

            curNode->Collected = false;

            // Tick -> if changed: OnNodePulse -> adds child nodes to the queue
            turn.Tasks.run(std::bind(&ParNode::Tick, curNode, &turn));

            if (turn.ScheduledNodes.Empty())
                break;

            curNode = turn.ScheduledNodes.Top();
        }
        while (curNode->Level == currentLevel);

        // Wait for tasks of current level
        turn.Tasks.wait();

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

    turn.ScheduledNodes.Invalidate();

    // Re-schedule this node
    node.Collected = true;
    turn.CollectBuffer.push_back(&node);
}

void PipeliningEngine::applyDynamicDetach(ParNode& node, ParNode& parent, PipeliningTurn& turn)
{
    OnNodeDetach(node, parent);
}

void PipeliningEngine::processChildren(ParNode& node, PipeliningTurn& turn)
{
    // Add children to queue
    for (auto* succ : node.Successors)
    {
        if (!succ->Collected.exchange(true))
            turn.CollectBuffer.push_back(succ);
    }
}

void PipeliningEngine::invalidateSuccessors(ParNode& node)
{
    for (auto* succ : node.Successors)
    {
        if (succ->NewLevel <= node.Level)
        {
            auto newLevel = node.Level + 1;
            succ->NewLevel = newLevel;

            if (succ->IsDynamicNode() && maxDynamicLevel_ < newLevel)
                maxDynamicLevel_ = newLevel;
        }
    }
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