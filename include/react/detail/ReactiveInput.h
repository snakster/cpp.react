
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

#include "ObserverBase.h"

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

enum
{
    allow_merging    = 1 << 0
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IContinuationTarget
///////////////////////////////////////////////////////////////////////////////////////////////////
struct IContinuationTarget
{
    virtual void DoContinuation(TransactionFuncT&&) = 0;
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
/// ContinuationData
///////////////////////////////////////////////////////////////////////////////////////////////////
template <bool is_thread_safe>
class ContinuationData
{
public:
    bool HasNext() const;

    void StoreContinuation(IContinuationTarget& target, TransactionFuncT&& cont);

    void RunNext();

    void QueueObserverForDetach(IObserver& obs);

    template <typename D>
    void DetachQueuedObservers();
};

// Not thread-safe
template <>
class ContinuationData<false>
{
    using ContDataT     = std::pair<IContinuationTarget*,TransactionFuncT>;
    using DataQueueT    = std::deque<ContDataT>;
    using ObsVectT      = std::vector<IObserver*>;

public:
    bool HasNext() const
    {
        return !storedContinuations_.empty();
    }

    void StoreContinuation(IContinuationTarget& target, TransactionFuncT&& cont)
    {
        storedContinuations_.emplace_back(&target, std::move(cont));
    }

    void RunNext()
    {
        ContDataT t = std::move(storedContinuations_.front());
        storedContinuations_.pop_front();

        t.first->DoContinuation(std::move(t.second));
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

// Thread-safe
template <>
class ContinuationData<true>
{
    using ContDataT     = std::pair<IContinuationTarget*,TransactionFuncT>;
    using DataQueueT    = std::deque<ContDataT>;
    using ObsVectT      = std::vector<IObserver*>;

public:
    bool HasNext() const
    {
        return contCount_ != 0;
    }

    void StoreContinuation(IContinuationTarget& target, TransactionFuncT&& cont)
    {
        storedContinuations_.local().emplace_back(&target, std::move(cont));
        ++contCount_;
    }

    void RunNext()
    {
        for (auto& v : storedContinuations_)
        {
            if (v.size() == 0)
                continue;

            ContDataT t = std::move(v.front());
            v.pop_front();
            contCount_--;

            t.first->DoContinuation(std::move(t.second));
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
    using ContinuationT = ContinuationData<D::is_parallel>;

    struct TransactionData
    {
        std::vector<IInputNode*>    ChangedInputs;
    };

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
        if (canMerge && Engine::TryMergeSync(std::forward<F>(func)))
            return;

        bool shouldPropagate = false;

        TurnT turn( nextTurnId(), flags );

        Engine::EnterTurnQueue(turn);

        // Phase 1 - Input admission
        ThreadLocalInputState<>::IsTransactionActive = true;

        Engine::OnTurnAdmissionStart(turn);
        func();
        Engine::ApplyMergedInputs(turn);
        Engine::OnTurnAdmissionEnd(turn);

        ThreadLocalInputState<>::IsTransactionActive = false;

        // Phase 2 - Apply input node changes
        for (auto* p : transaction_.ChangedInputs)
            if (p->ApplyInput(&turn))
                shouldPropagate = true;
        transaction_.ChangedInputs.clear();

        // Phase 3 - Propagate changes
        if (shouldPropagate)
            Engine::Propagate(turn);

        Engine::ExitTurnQueue(turn);

        processContinuationData(flags);
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
    virtual void DoContinuation(TransactionFuncT&& cont) override
    {
        DoTransaction(0, std::move(cont));
    }
        
    virtual void AsyncContinuation(TransactionFuncT&& cont) override
    {
        AsyncTransaction(0, nullptr, std::move(cont));
    }
    //~IContinuationTarget

    ContinuationT& Continuation()
    {
        return continuation_;
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
        TurnT turn( nextTurnId(), 0 );

        Engine::EnterTurnQueue(turn);

        Engine::OnTurnAdmissionStart(turn);
        r.AddInput(std::forward<V>(v));
        Engine::ApplyMergedInputs(turn);
        Engine::OnTurnAdmissionEnd(turn);

        if (r.ApplyInput(&turn))
            Engine::Propagate(turn);

        Engine::ExitTurnQueue(turn);

        processContinuationData(0);
    }

    template <typename R, typename F>
    void modifySimpleInput(R& r, const F& func)
    {
        TurnT turn( nextTurnId(), 0 );

        Engine::EnterTurnQueue(turn);

        Engine::OnTurnAdmissionStart(turn);
        r.ModifyInput(func);
        Engine::ApplyMergedInputs(turn);
        Engine::OnTurnAdmissionEnd(turn);

        // Return value, will always be true
        r.ApplyInput(&turn);

        Engine::Propagate(turn);

        Engine::ExitTurnQueue(turn);

        processContinuationData(0);
    }

    // This input is part of an active transaction
    template <typename R, typename V>
    void addTransactionInput(R& r, V&& v)
    {
        r.AddInput(std::forward<V>(v));
        transaction_.ChangedInputs.push_back(&r);
    }

    template <typename R, typename F>
    void modifyTransactionInput(R& r, const F& func)
    {
        r.ModifyInput(func);
        transaction_.ChangedInputs.push_back(&r);
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
            if (canMerge && Engine::TryMergeAsync(std::move(item.Func), std::move(item.StatusPtr)))
                continue;

            bool shouldPropagate = false;

            TurnT turn( nextTurnId(), item.Flags );

            // Blocks until turn is at the front of the queue
            Engine::EnterTurnQueue(turn);

            // Phase 1 - Input admission
            ThreadLocalInputState<>::IsTransactionActive = true;

            Engine::OnTurnAdmissionStart(turn);

            // Input of current item
            item.Func();

            // Merged inputs that arrived while this item was queued
            Engine::ApplyMergedInputs(turn);

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
            for (auto* p : transaction_.ChangedInputs)
                if (p->ApplyInput(&turn))
                    shouldPropagate = true;
            transaction_.ChangedInputs.clear();

            // Phase 3 - Propagate changes
            if (shouldPropagate)
                Engine::Propagate(turn);

            Engine::ExitTurnQueue(turn);

            processContinuationData(savedFlags);

            // Decrement transaction status counts of processed items
            if (savedStatusPtr != nullptr)
                savedStatusPtr->DecWaitCount();

            for (auto& p : statusPtrStash)
                p->DecWaitCount();
            statusPtrStash.clear();
        }
    }

    void processContinuationData(TurnFlagsT flags)
    {
        // No merging for continuations
        flags &= ~allow_merging;

        continuation_.template DetachQueuedObservers<D>();

        while (continuation_.HasNext())
        {
            continuation_.RunNext();
            continuation_.template DetachQueuedObservers<D>();
        }
    }

    AsyncQueueT             asyncQueue_;
    std::thread             asyncWorker_;

    std::atomic<TurnIdT>    nextTurnId_{ 0 };

    TransactionData         transaction_;
    ContinuationT           continuation_;
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