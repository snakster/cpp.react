
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_REACTIVEINPUT_H_INCLUDED
#define REACT_DETAIL_REACTIVEINPUT_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <utility>
#include <thread>
#include <vector>

#include "tbb/concurrent_queue.h"
#include "tbb/enumerable_thread_specific.h"
#include "tbb/queuing_mutex.h"

#include "IReactiveEngine.h"
#include "ObserverBase.h"
#include "react/common/Concurrency.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward declarations
///////////////////////////////////////////////////////////////////////////////////////////////////
struct IInputNode;
class IObserver;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Common types & constants
///////////////////////////////////////////////////////////////////////////////////////////////////
using TurnIdT = uint;
using TurnFlagsT = uint;
using TransactionFuncT = std::function<void()>;

enum ETurnFlags
{
    allow_merging    = 1 << 0
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EInputMode
///////////////////////////////////////////////////////////////////////////////////////////////////
enum EInputMode
{
    consecutive_input,
    concurrent_input
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IContinuationTarget
///////////////////////////////////////////////////////////////////////////////////////////////////
struct IContinuationTarget
{
    virtual void AsyncContinuation(TransactionFuncT&&) = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ThreadLocalInputState
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename = void>
struct ThreadLocalInputState
{
    static REACT_TLS bool   IsTransactionActive;
};

template <typename T>
REACT_TLS bool ThreadLocalInputState<T>::IsTransactionActive(false);

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ContinuationManager
///////////////////////////////////////////////////////////////////////////////////////////////////
// Interface
template <EPropagationMode>
class ContinuationManager
{
public:
    void StoreContinuation(IContinuationTarget& target, TransactionFuncT&& cont);

    bool HasNext() const;
    void ProcessNext();

    void QueueObserverForDetach(IObserver& obs);

    template <typename D>
    void DetachQueuedObservers();
};

// Non thread-safe implementation
template <>
class ContinuationManager<sequential_propagation>
{
    using ContDataT     = std::pair<IContinuationTarget*,TransactionFuncT>;
    using DataQueueT    = std::deque<ContDataT>;
    using ObsVectT      = std::vector<IObserver*>;

public:
    void StoreContinuation(IContinuationTarget& target, TransactionFuncT&& cont)
    {
        storedContinuations_.emplace_back(&target, std::move(cont));
    }

    bool HasNext() const
    {
        return !storedContinuations_.empty();
    }

    void ProcessNext()
    {
        ContDataT t = std::move(storedContinuations_.front());
        storedContinuations_.pop_front();

        t.first->AsyncContinuation(std::move(t.second));
    }

    // Todo: Move this somewhere else
    void QueueObserverForDetach(IObserver& obs)
    {
        detachedObservers_.push_back(&obs);
    }

    template <typename D>
    void DetachQueuedObservers()
    {
        auto& registry = DomainSpecificObserverRegistry<D>::Instance();

        for (auto* o : detachedObservers_)
            registry.Unregister(o);
        detachedObservers_.clear();
    }

private:
    DataQueueT  storedContinuations_;
    ObsVectT    detachedObservers_;
};

// Thread-safe implementation
template <>
class ContinuationManager<parallel_propagation>
{
    using ContDataT     = std::pair<IContinuationTarget*,TransactionFuncT>;
    using DataQueueT    = std::deque<ContDataT>;
    using ObsVectT      = std::vector<IObserver*>;

public:
    void StoreContinuation(IContinuationTarget& target, TransactionFuncT&& cont)
    {
        storedContinuations_.local().emplace_back(&target, std::move(cont));
        ++contCount_;
    }

    bool HasNext() const
    {
        return contCount_ != 0;
    }

    void ProcessNext()
    {
        for (auto& v : storedContinuations_)
        {
            if (v.size() == 0)
                continue;

            ContDataT t = std::move(v.front());
            v.pop_front();
            contCount_--;

            t.first->AsyncContinuation(std::move(t.second));
            break;
        }
    }

    void QueueObserverForDetach(IObserver& obs)
    {
        detachedObservers_.local().push_back(&obs);
    }

    template <typename D>
    void DetachQueuedObservers()
    {
        auto& registry = DomainSpecificObserverRegistry<D>::Instance();

        for (auto& v : detachedObservers_)
        {
            for (auto* o : v)
                registry.Unregister(o);
            v.clear();
        }
    }

private:
    tbb::enumerable_thread_specific<DataQueueT> storedContinuations_;
    tbb::enumerable_thread_specific<ObsVectT>   detachedObservers_;

    size_t contCount_ = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// AsyncState
///////////////////////////////////////////////////////////////////////////////////////////////////
class AsyncState
{
public:
    inline void Wait()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return !isWaiting_; });
    }

    inline void IncWaitCount()
    {
        auto oldVal = waitCount_.fetch_add(1, std::memory_order_relaxed);

        if (oldVal == 0)
        {// mutex_
            std::lock_guard<std::mutex> scopedLock(mutex_);
            isWaiting_ = true;
        }// ~mutex_
    }

    inline void DecWaitCount()
    {
        auto oldVal = waitCount_.fetch_sub(1, std::memory_order_relaxed);

        if (oldVal == 1)
        {// mutex_
            std::lock_guard<std::mutex> scopedLock(mutex_);
            isWaiting_ = false;
            condition_.notify_all();
        }// ~mutex_
    }

private:
    std::atomic<uint>           waitCount_{ 0 };
    std::condition_variable     condition_;
    std::mutex                  mutex_;
    bool                        isWaiting_ = false;
};

using AsyncStatePtrT = std::shared_ptr<AsyncState>;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// TransactionQueue
///////////////////////////////////////////////////////////////////////////////////////////////////
// Interface
template <EInputMode>
class TransactionQueue
{
public:
    class QueueEntry
    {
    public:
        explicit QueueEntry(TurnFlagsT flags);
        void RunMergedInputs();
    };

    template <typename F>
    bool TryMergeSync(F&& inputFunc);

    template <typename F>
    bool TryMergeAsync(F&& inputFunc, AsyncStatePtrT&& statusPtr);

    void EnterQueue(QueueEntry& turn);
    void ExitQueue(QueueEntry& turn);
};

// Non thread-safe implementation
template <>
class TransactionQueue<consecutive_input>
{
public:
    class QueueEntry
    {
    public:
        explicit QueueEntry(TurnFlagsT flags)   {}
        void RunMergedInputs()                  {}
    };

    template <typename F>
    bool TryMergeSync(F&& inputFunc) { return false; }

    template <typename F>
    bool TryMergeAsync(F&& inputFunc, AsyncStatePtrT&& statusPtr) { return false; }

    void EnterQueue(QueueEntry& turn)   {}
    void ExitQueue(QueueEntry& turn)    {}
};

// Thread-safe implementation
template <>
class TransactionQueue<concurrent_input>
{
public:
    class QueueEntry
    {
    public:
        explicit QueueEntry(TurnFlagsT flags) :
            isMergeable_( (flags & allow_merging) != 0 )
        {}

        void Append(QueueEntry& tr)
        {
            successor_ = &tr;
            tr.blockCondition_.Block();
        }

        void WaitForUnblock()
        {
            blockCondition_.WaitForUnblock();
        }

        void RunMergedInputs() const
        {
            for (const auto& e : merged_)
                e.InputFunc();
        }

        void UnblockSuccessors()
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
                else if (e.StatusPtr != nullptr)
                    e.StatusPtr->DecWaitCount();
            }

            // Release next thread in queue
            if (successor_)
                successor_->blockCondition_.Unblock();
        }

        template <typename F>
        bool TryMerge(F&& inputFunc, BlockingCondition* caller, AsyncStatePtrT&& statusPtr)
        {
            if (!isMergeable_)
                return false;

            // Only merge if target is still blocked
            bool merged = blockCondition_.RunIfBlocked([&] {
                if (caller)
                    caller->Block();
                merged_.emplace_back(std::forward<F>(inputFunc), caller, std::move(statusPtr));
            });

            return merged;
        }

    private:
        struct MergedData
        {
            template <typename F>
            MergedData(F&& func, BlockingCondition* cond, AsyncStatePtrT&& statusPtr) :
                InputFunc( std::forward<F>(func) ),
                Cond( cond ),
                StatusPtr( std::move(statusPtr) )
            {}

            std::function<void()>   InputFunc;

            // Blocking condition variable for sync merged
            BlockingCondition*      Cond;  

            // Status for async merged
            AsyncStatePtrT          StatusPtr;
        };

        using MergedDataVectT = std::vector<MergedData>;

        bool                isMergeable_;
        QueueEntry*         successor_ = nullptr;
        MergedDataVectT     merged_;
        BlockingCondition   blockCondition_;
    };

    template <typename F>
    bool TryMergeSync(F&& inputFunc)
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
    bool TryMergeAsync(F&& inputFunc, AsyncStatePtrT&& statusPtr)
    {
        bool merged = false;

        {// seqMutex_
            SeqMutexT::scoped_lock lock(seqMutex_);

            if (tail_)
                merged = tail_->TryMerge(
                    std::forward<F>(inputFunc), nullptr, std::move(statusPtr));
        }// ~seqMutex_

        return merged;
    }

    void EnterQueue(QueueEntry& turn)
    {
        {// seqMutex_
            SeqMutexT::scoped_lock lock(seqMutex_);

            if (tail_)
                tail_->Append(turn);

            tail_ = &turn;
        }// ~seqMutex_

        turn.WaitForUnblock();
    }

    void ExitQueue(QueueEntry& turn)
    {// seqMutex_
        SeqMutexT::scoped_lock lock(seqMutex_);

        turn.UnblockSuccessors();

        if (tail_ == &turn)
            tail_ = nullptr;
    }// ~seqMutex_

private:
    using SeqMutexT = tbb::queuing_mutex;

    SeqMutexT       seqMutex_;
    QueueEntry*     tail_       = nullptr;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// InputManager
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class InputManager :
    public IContinuationTarget
{
private:
    struct AsyncItem
    {
        TurnFlagsT              Flags;
        AsyncStatePtrT          StatusPtr;
        TransactionFuncT        Func;
    };

    using AsyncQueueT = tbb::concurrent_bounded_queue<AsyncItem>;

    // Select between thread-safe and non thread-safe implementations
    using TransactionQueueT = TransactionQueue<D::Policy::input_mode>;
    using ContinuationManagerT = ContinuationManager<D::Policy::propagation_mode>;

public:
    using TurnT = typename D::TurnT;
    using Engine = typename D::Engine;

    InputManager() :
        asyncWorker_( [this] { processAsyncQueue(); } )
    {
        asyncWorker_.detach();
    }

    template <typename F>
    void DoTransaction(TurnFlagsT flags, F&& func)
    {
        // Attempt to add input to another turn.
        // If successful, blocks until other turn is done and returns.
        bool canMerge = (flags & allow_merging) != 0;
        if (canMerge && transactionQueue_.TryMergeSync(std::forward<F>(func)))
            return;

        bool shouldPropagate = false;
        
        typename TransactionQueueT::QueueEntry tr( flags );
        transactionQueue_.EnterQueue(tr);

        // Phase 1 - Input admission
        ThreadLocalInputState<>::IsTransactionActive = true;

        TurnT turn( nextTurnId(), flags );
        Engine::OnTurnAdmissionStart(turn);
        func();
        tr.RunMergedInputs();
        Engine::OnTurnAdmissionEnd(turn);

        ThreadLocalInputState<>::IsTransactionActive = false;

        // Phase 2 - Apply input node changes
        for (auto* p : changedInputs_)
            if (p->ApplyInput(&turn))
                shouldPropagate = true;
        changedInputs_.clear();

        // Phase 3 - Propagate changes
        if (shouldPropagate)
            Engine::Propagate(turn);

        transactionQueue_.ExitQueue(tr);

        processContinuations(flags);
    }

    template <typename F>
    void AsyncTransaction(TurnFlagsT flags, const AsyncStatePtrT& statusPtr, F&& func)
    {
        if (statusPtr != nullptr)
            statusPtr->IncWaitCount();

        asyncQueue_.push(AsyncItem{ flags, statusPtr, std::forward<F>(func) } );
    }

    template <typename R, typename V>
    void AddInput(R& r, V&& v)
    {
        if (ThreadLocalInputState<>::IsTransactionActive)
        {
            addTransactionInput(r, std::forward<V>(v));
        }
        else
        {
            addSimpleInput(r, std::forward<V>(v));
        }
    }

    template <typename R, typename F>
    void ModifyInput(R& r, const F& func)
    {
        if (ThreadLocalInputState<>::IsTransactionActive)
        {
            modifyTransactionInput(r, func);
        }
        else
        {
            modifySimpleInput(r, func);
        }
    }

    //IContinuationTarget        
    virtual void AsyncContinuation(TransactionFuncT&& cont) override
    {
        DoTransaction(0, std::move(cont));
        // Todo: fixme
        //AsyncTransaction(0, nullptr, std::move(cont));
    }
    //~IContinuationTarget

    void StoreContinuation(IContinuationTarget& target, TransactionFuncT&& cont)
    {
        continuationManager_.StoreContinuation(target, std::move(cont));
    }

    void QueueObserverForDetach(IObserver& obs)
    {
        continuationManager_.QueueObserverForDetach(obs);
    }

private:
    TurnIdT nextTurnId()
    {
        auto curId = nextTurnId_.fetch_add(1, std::memory_order_relaxed);

        if (curId == (std::numeric_limits<int>::max)())
            nextTurnId_.fetch_sub((std::numeric_limits<int>::max)());

        return curId;
    }

    // Create a turn with a single input
    template <typename R, typename V>
    void addSimpleInput(R& r, V&& v)
    {
        typename TransactionQueueT::QueueEntry tr( 0 );

        transactionQueue_.EnterQueue(tr);

        TurnT turn( nextTurnId(), 0 );
        Engine::OnTurnAdmissionStart(turn);
        r.AddInput(std::forward<V>(v));
        tr.RunMergedInputs();
        Engine::OnTurnAdmissionEnd(turn);

        if (r.ApplyInput(&turn))
            Engine::Propagate(turn);

        transactionQueue_.ExitQueue(tr);

        processContinuations(0);
    }

    template <typename R, typename F>
    void modifySimpleInput(R& r, const F& func)
    {
        typename TransactionQueueT::QueueEntry tr( 0 );

        transactionQueue_.EnterQueue(tr);

        TurnT turn( nextTurnId(), 0 );
        Engine::OnTurnAdmissionStart(turn);
        r.ModifyInput(func);
        Engine::OnTurnAdmissionEnd(turn);

        // Return value, will always be true
        r.ApplyInput(&turn);

        Engine::Propagate(turn);

        transactionQueue_.ExitQueue(tr);

        processContinuations(0);
    }

    // This input is part of an active transaction
    template <typename R, typename V>
    void addTransactionInput(R& r, V&& v)
    {
        r.AddInput(std::forward<V>(v));
        changedInputs_.push_back(&r);
    }

    template <typename R, typename F>
    void modifyTransactionInput(R& r, const F& func)
    {
        r.ModifyInput(func);
        changedInputs_.push_back(&r);
    }

    void processAsyncQueue()
    {
        AsyncItem item;

        AsyncStatePtrT  savedStatusPtr = nullptr;
        TurnFlagsT      savedFlags = 0;

        std::vector<AsyncStatePtrT> statusPtrStash;

        bool skipPop = false;

        while (true)
        {
            // Blocks if queue is empty
            if (!skipPop)
                asyncQueue_.pop(item);
            else
                skipPop = false;

            // First try to merge to an existing synchronous item in the queue
            bool canMerge = (item.Flags & allow_merging) != 0;
            if (canMerge && transactionQueue_.TryMergeAsync(
                    std::move(item.Func),
                    std::move(item.StatusPtr)))
                continue;

            bool shouldPropagate = false;

            // Blocks until turn is at the front of the queue
            typename TransactionQueueT::QueueEntry tr( item.Flags );
            transactionQueue_.EnterQueue(tr);

            TurnT turn( nextTurnId(), item.Flags );

            // Phase 1 - Input admission
            ThreadLocalInputState<>::IsTransactionActive = true;

            Engine::OnTurnAdmissionStart(turn);

            // Input of current item
            item.Func();

            // Merged inputs that arrived while this item was queued
            tr.RunMergedInputs();

            // Save data, because item might be re-used for next input
            savedStatusPtr = std::move(item.StatusPtr);
            savedFlags = item.Flags;

            // If the current item supports merging, try to add more mergeable inputs
            // to this turn
            if (canMerge)
            {
                uint extraCount = 0;
                while (extraCount < 100 && asyncQueue_.try_pop(item))
                {
                    bool canMergeNext = (item.Flags & allow_merging) != 0;
                    if (canMergeNext)
                    {
                        item.Func();
                        if (item.StatusPtr != nullptr)
                            statusPtrStash.push_back(std::move(item.StatusPtr));

                        ++extraCount;
                    }
                    else
                    {
                        // We already popped an item we could not merge
                        // Process it in the next iteration
                        skipPop = true;

                        // Break at first item that cannot be merged.
                        // We only allow merging of continuous ranges.
                        break;
                    }
                }
            }

            Engine::OnTurnAdmissionEnd(turn);

            ThreadLocalInputState<>::IsTransactionActive = false;

            // Phase 2 - Apply input node changes
            for (auto* p : changedInputs_)
                if (p->ApplyInput(&turn))
                    shouldPropagate = true;
            changedInputs_.clear();

            // Phase 3 - Propagate changes
            if (shouldPropagate)
                Engine::Propagate(turn);

            transactionQueue_.ExitQueue(tr);

            processContinuations(savedFlags);

            // Decrement transaction status counts of processed items
            if (savedStatusPtr != nullptr)
                savedStatusPtr->DecWaitCount();

            for (auto& p : statusPtrStash)
                p->DecWaitCount();
            statusPtrStash.clear();
        }
    }

    void processContinuations(TurnFlagsT flags)
    {
        // No merging for continuations
        flags &= ~allow_merging;

        continuationManager_.template DetachQueuedObservers<D>();

        while (continuationManager_.HasNext())
        {
            continuationManager_.ProcessNext();
            continuationManager_.template DetachQueuedObservers<D>();
        }
    }

    TransactionQueueT       transactionQueue_;
    ContinuationManagerT    continuationManager_;

    AsyncQueueT             asyncQueue_;
    std::thread             asyncWorker_;

    std::atomic<TurnIdT>    nextTurnId_{ 0 };

    std::vector<IInputNode*>    changedInputs_;
};

template <typename D>
class DomainSpecificInputManager
{
public:
    DomainSpecificInputManager() = delete;

    static InputManager<D>& Instance()
    {
        static InputManager<D> instance;
        return instance;
    }
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_REACTIVEINPUT_H_INCLUDED