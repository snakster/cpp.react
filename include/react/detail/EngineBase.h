
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_ENGINEBASE_H_INCLUDED
#define REACT_DETAIL_ENGINEBASE_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "tbb/queuing_mutex.h"

#include "react/common/Concurrency.h"
#include "react/common/Types.h"
#include "react/detail/ReactiveInput.h"
#include "react/detail/IReactiveNode.h"
#include "react/detail/IReactiveEngine.h"
#include "react/detail/ObserverBase.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// TurnBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <bool is_thread_safe>
class TurnBase
{
public:
    TurnBase(TurnIdT id, TurnFlagsT flags) :
        id_( id )
    {}

    TurnIdT Id() const { return id_; }

private:
    TurnIdT    id_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// TurnQueueManager
///////////////////////////////////////////////////////////////////////////////////////////////////
class TurnQueueManager
{
public:
    class QueueEntry
    {
    public:
        explicit QueueEntry(TurnFlagsT flags) :
            isMergeable_( (flags & enable_input_merging) != 0 )
        {}

        inline void Append(QueueEntry& tr)
        {
            successor_ = &tr;
            tr.blockCondition_.Block();
        }

        inline void WaitForUnblock()
        {
            blockCondition_.WaitForUnblock();
        }

        inline void RunMergedInputs() const
        {
            for (const auto& e : merged_)
                e.InputFunc();
        }

        inline void UnblockSuccessors()
        {
            // Release merged
            for (const auto& e : merged_)
            {
                // Note: Since a merged input is either sync or async,
                // either cond or status will be null

                // Sync
                if (e.Cond != nullptr)
                    e.Cond->Unblock();

                // Async
                else if (e.Status != nullptr)
                    TransactionStatusInterface::DecrementWaitCount(*e.Status);
            }

            // Release next thread in queue
            if (successor_)
                successor_->blockCondition_.Unblock();
        }

        template <typename F>
        inline bool TryMerge(F&& inputFunc, BlockingCondition* caller, TransactionStatus* status)
        {
            if (!isMergeable_)
                return false;

            // Only merge if target is still blocked
            bool merged = blockCondition_.RunIfBlocked([&] {
                if (caller)
                    caller->Block();
                merged_.emplace_back(std::forward<F>(inputFunc), caller, status);
            });

            return merged;
        }

    private:
        struct MergedData
        {
            template <typename F>
            MergedData(F&& func, BlockingCondition* cond, TransactionStatus* status) :
                InputFunc( std::forward<F>(func) ),
                Cond( cond ),
                Status( status )
            {}

            std::function<void()>   InputFunc;

            // Blocking condition variable for sync merged
            BlockingCondition*      Cond;  

            // Status for async merged
            TransactionStatus*      Status;
        };

        using MergedDataVectT = std::vector<MergedData>;

        bool                isMergeable_;
        QueueEntry*         successor_ = nullptr;
        MergedDataVectT     merged_;
        BlockingCondition   blockCondition_;
    };

    template <typename F>
    inline bool TryMergeSync(F&& inputFunc)
    {
        bool merged = false;

        BlockingCondition caller;

        {// seqMutex_
            SeqMutexT::scoped_lock lock(seqMutex_);

            if (tail_)
                merged = tail_->TryMerge(std::forward<F>(inputFunc), &caller, nullptr);
        }// ~seqMutex_

        if (merged)
            caller.WaitForUnblock();

        return merged;
    }

    template <typename F>
    inline bool TryMergeAsync(F&& inputFunc, TransactionStatus* status)
    {
        bool merged = false;

        {// seqMutex_
            SeqMutexT::scoped_lock lock(seqMutex_);

            if (tail_)
                merged = tail_->TryMerge(std::forward<F>(inputFunc), nullptr, status);
        }// ~seqMutex_

        return merged;
    }

    inline void EnterQueue(QueueEntry& turn)
    {
        {// seqMutex_
            SeqMutexT::scoped_lock lock(seqMutex_);

            if (tail_)
                tail_->Append(turn);

            tail_ = &turn;
        }// ~seqMutex_

        turn.WaitForUnblock();
    }

    inline void ExitQueue(QueueEntry& turn)
    {// seqMutex_
        SeqMutexT::scoped_lock lock(seqMutex_);

        turn.UnblockSuccessors();

        if (tail_ == &turn)
            tail_ = nullptr;
    }// ~seqMutex_

private:
    using SeqMutexT = tbb::queuing_mutex;

    SeqMutexT       seqMutex_;
    QueueEntry*     tail_ = nullptr;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// DefaultQueueableTurn
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurnBase>
class DefaultQueueableTurn :
    public TTurnBase,
    public TurnQueueManager::QueueEntry
{
public:
    DefaultQueueableTurn(TurnIdT id, TurnFlagsT flags) :
        TTurnBase( id, flags ),
        TurnQueueManager::QueueEntry( flags )
    {}
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// DefaultQueuingEngine
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    template <typename Turn_> class TTEngineBase,
    typename TTurnBase
>
class DefaultQueuingEngine : public TTEngineBase<DefaultQueueableTurn<TTurnBase>>
{
public:
    using TurnT = DefaultQueueableTurn<TTurnBase>;

    template <typename F>
    bool TryMergeSync(F&& f)
    {
        return queueManager_.TryMergeSync(std::forward<F>(f));
    }

    template <typename F>
    bool TryMergeAsync(F&& f, TransactionStatus* statusPtr)
    {
        return queueManager_.TryMergeAsync(std::forward<F>(f), statusPtr);
    }

    void ApplyMergedInputs(TurnT& turn)
    {
        turn.RunMergedInputs();
    }

    void EnterTurnQueue(TurnT& turn)
    {
        queueManager_.EnterQueue(turn);
    }
    
    void ExitTurnQueue(TurnT& turn)
    {
        queueManager_.ExitQueue(turn);
    }

private:
    TurnQueueManager        queueManager_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_ENGINEBASE_H_INCLUDED