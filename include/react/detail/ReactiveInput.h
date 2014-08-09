
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
#include <type_traits>
#include <vector>

#include "tbb/task.h"
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

template <typename D>
class InputManager;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Common types & constants
///////////////////////////////////////////////////////////////////////////////////////////////////
using TurnIdT = uint;
using TransactionFuncT = std::function<void()>;

enum ETransactionFlags
{
    allow_merging    = 1 << 0
};

using TransactionFlagsT = std::underlying_type<ETransactionFlags>::type;

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
    virtual void AsyncContinuation(TransactionFlagsT, const WaitingStatePtrT&,
                                   TransactionFuncT&&) = 0;
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
    void StoreContinuation(IContinuationTarget& target, TransactionFlagsT flags,
                           TransactionFuncT&& cont);

    bool HasContinuations() const;
    void StartContinuations(const WaitingStatePtrT& waitingStatePtr);

    void QueueObserverForDetach(IObserver& obs);

    template <typename D>
    void DetachQueuedObservers();
};

// Non thread-safe implementation
template <>
class ContinuationManager<sequential_propagation>
{
    struct Data_
    {
        Data_(IContinuationTarget* target, TransactionFlagsT flags, TransactionFuncT&& func) :
            Target( target ),
            Flags( flags ),
            Func( func )
        {}

        IContinuationTarget*    Target;
        TransactionFlagsT       Flags;
        TransactionFuncT        Func;
    };

    using DataVectT = std::vector<Data_>;
    using ObsVectT  = std::vector<IObserver*>;

public:
    void StoreContinuation(IContinuationTarget& target, TransactionFlagsT flags,
                           TransactionFuncT&& cont)
    {
        storedContinuations_.emplace_back(&target, flags, std::move(cont));
    }

    bool HasContinuations() const
    {
        return !storedContinuations_.empty();
    }

    void StartContinuations(const WaitingStatePtrT& waitingStatePtr)
    {
        for (auto& t : storedContinuations_)
            t.Target->AsyncContinuation(t.Flags, waitingStatePtr, std::move(t.Func));

        storedContinuations_.clear();
    }

    // Todo: Move this somewhere else
    void QueueObserverForDetach(IObserver& obs)
    {
        detachedObservers_.push_back(&obs);
    }

    template <typename D>
    void DetachQueuedObservers()
    {
        for (auto* o : detachedObservers_)
            o->UnregisterSelf();
        detachedObservers_.clear();
    }

private:
    DataVectT   storedContinuations_;
    ObsVectT    detachedObservers_;
};

// Thread-safe implementation
template <>
class ContinuationManager<parallel_propagation>
{
    struct Data_
    {
        Data_(IContinuationTarget* target, TransactionFlagsT flags, TransactionFuncT&& func) :
            Target( target ),
            Flags( flags ),
            Func( func )
        {}

        IContinuationTarget*    Target;
        TransactionFlagsT       Flags;
        TransactionFuncT        Func;
    };

    using DataVecT  = std::vector<Data_>;
    using ObsVectT  = std::vector<IObserver*>;

public:
    void StoreContinuation(IContinuationTarget& target, TransactionFlagsT flags,
                           TransactionFuncT&& cont)
    {
        storedContinuations_.local().emplace_back(&target, flags, std::move(cont));
        ++contCount_;
    }

    bool HasContinuations() const
    {
        return contCount_ != 0;
    }

    void StartContinuations(const WaitingStatePtrT& waitingStatePtr)
    {
        for (auto& v : storedContinuations_)
            for (auto& t : v)
                t.Target->AsyncContinuation(t.Flags, waitingStatePtr, std::move(t.Func));

        storedContinuations_.clear();
        contCount_ = 0;
    }

    void QueueObserverForDetach(IObserver& obs)
    {
        detachedObservers_.local().push_back(&obs);
    }

    template <typename D>
    void DetachQueuedObservers()
    {
        for (auto& v : detachedObservers_)
        {
            for (auto* o : v)
                o->UnregisterSelf();
            v.clear();
        }
    }

private:
    tbb::enumerable_thread_specific<DataVecT>   storedContinuations_;
    tbb::enumerable_thread_specific<ObsVectT>   detachedObservers_;

    size_t contCount_ = 0;
};

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
        explicit QueueEntry(TransactionFlagsT flags);

        void RunMergedInputs();

        size_t  GetWaitingStatePtrCount() const;
        void    MoveWaitingStatePtrs(std::vector<WaitingStatePtrT>& out);

        void Release();

        bool IsAsync() const;
    };

    template <typename F>
    bool TryMergeSync(F&& inputFunc);

    template <typename F>
    bool TryMergeAsync(F&& inputFunc, WaitingStatePtrT&& waitingStatePtr);

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
        explicit QueueEntry(TransactionFlagsT flags) {}

        void RunMergedInputs()  {}

        size_t  GetWaitingStatePtrCount() const    { return 0; }
        void    MoveWaitingStatePtrs(std::vector<WaitingStatePtrT>& out)  {}

        void Release()  {}

        bool IsAsync() const { return false; }
    };

    template <typename F>
    bool TryMergeSync(F&& inputFunc) { return false; }

    template <typename F>
    bool TryMergeAsync(F&& inputFunc, WaitingStatePtrT&& waitingStatePtr) { return false; }

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
        explicit QueueEntry(TransactionFlagsT flags) :
            isMergeable_( (flags & allow_merging) != 0 )
        {}

        void Append(QueueEntry& tr)
        {
            successor_ = &tr;
            tr.waitingState_.IncWaitCount();
            ++waitingStatePtrCount_;
        }

        void WaitForUnblock()
        {
            waitingState_.Wait();
        }

        void RunMergedInputs() const
        {
            for (const auto& e : merged_)
                e.InputFunc();
        }

        size_t GetWaitingStatePtrCount() const
        {
            return waitingStatePtrCount_;
        }

        void MoveWaitingStatePtrs(std::vector<WaitingStatePtrT>& out)
        {
            if (waitingStatePtrCount_ == 0)
                return;

            for (const auto& e : merged_)
                if (e.WaitingStatePtr != nullptr)
                    out.push_back(std::move(e.WaitingStatePtr));

            if (successor_)
            {
                out.push_back(WaitingStatePtrT( &successor_->waitingState_ ));

                // Ownership of successors waiting state has been transfered,
                // we no longer need it
                successor_ = nullptr;
            }

            waitingStatePtrCount_ = 0;
        }

        void Release()
        {
            if (waitingStatePtrCount_ == 0)
                return;

            // Release merged
            for (const auto& e : merged_)
                if (e.WaitingStatePtr != nullptr)
                    e.WaitingStatePtr->DecWaitCount();

            // Waiting state ptrs may point to stack of waiting threads.
            // After decwaitcount, they may start running and terminate.
            // Thus, clear merged ASAP because it now contains invalid ptrs.
            merged_.clear();

            // Release next thread in queue
            if (successor_)
                successor_->waitingState_.DecWaitCount();
        }

        template <typename F, typename P>
        bool TryMerge(F&& inputFunc, P&& waitingStatePtr)
        {
            if (!isMergeable_)
                return false;

            // Only merge if target is still waiting
            bool merged = waitingState_.RunIfWaiting([&] {
                if (waitingStatePtr != nullptr)
                {
                    waitingStatePtr->IncWaitCount();
                    ++waitingStatePtrCount_;
                }

                merged_.emplace_back(std::forward<F>(inputFunc), std::forward<P>(waitingStatePtr));
            });

            return merged;
        }

    private:
        struct MergedData
        {
            template <typename F, typename P>
            MergedData(F&& func, P&& waitingStatePtr) :
                InputFunc( std::forward<F>(func) ),
                WaitingStatePtr( std::forward<P>(waitingStatePtr) )
            {}

            std::function<void()>   InputFunc;
            WaitingStatePtrT        WaitingStatePtr;
        };

        using MergedDataVectT = std::vector<MergedData>;

        bool                isMergeable_;
        QueueEntry*         successor_      = nullptr;
        MergedDataVectT     merged_;
        UniqueWaitingState  waitingState_;
        size_t              waitingStatePtrCount_ = 0;
    };

    template <typename F>
    bool TryMergeSync(F&& inputFunc)
    {
        bool merged = false;

        UniqueWaitingState st;
        WaitingStatePtrT p( &st );

        {// seqMutex_
            SeqMutexT::scoped_lock lock(seqMutex_);

            if (tail_)
                merged = tail_->TryMerge(std::forward<F>(inputFunc), p);
        }// ~seqMutex_

        if (merged)
            p->Wait();

        return merged;
    }

    template <typename F>
    bool TryMergeAsync(F&& inputFunc, WaitingStatePtrT&& waitingStatePtr)
    {
        bool merged = false;

        {// seqMutex_
            SeqMutexT::scoped_lock lock(seqMutex_);

            if (tail_)
                merged = tail_->TryMerge(std::forward<F>(inputFunc), std::move(waitingStatePtr));
        }// ~seqMutex_

        return merged;
    }

    void EnterQueue(QueueEntry& tr)
    {
        {// seqMutex_
            SeqMutexT::scoped_lock lock(seqMutex_);

            if (tail_)
                tail_->Append(tr);

            tail_ = &tr;
        }// ~seqMutex_

        tr.WaitForUnblock();
    }

    void ExitQueue(QueueEntry& tr)
    {
        {// seqMutex_
            SeqMutexT::scoped_lock lock(seqMutex_);

            if (tail_ == &tr)
                tail_ = nullptr;
        }// ~seqMutex_

        tr.Release();
    }

private:
    using SeqMutexT = tbb::queuing_mutex;

    SeqMutexT       seqMutex_;
    QueueEntry*     tail_       = nullptr;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// AsyncWorker
///////////////////////////////////////////////////////////////////////////////////////////////////
struct AsyncItem
{
    TransactionFlagsT       Flags;
    WaitingStatePtrT        WaitingStatePtr;
    TransactionFuncT        Func;
};

// Interface
template <typename D, EInputMode>
class AsyncWorker
{
public:
    AsyncWorker(InputManager<D>& mgr);

    void PushItem(AsyncItem&& item);

    void PopItem(AsyncItem& item);
    bool TryPopItem(AsyncItem& item);

    bool IncrementItemCount(size_t n);
    bool DecrementItemCount(size_t n);

    void Start();
};

// Disabled
template <typename D>
struct AsyncWorker<D, consecutive_input>
{
public:
    AsyncWorker(InputManager<D>& mgr)
    {}

    void PushItem(AsyncItem&& item)     { assert(false); }

    void PopItem(AsyncItem& item)       { assert(false); }
    bool TryPopItem(AsyncItem& item)    { assert(false); return false; }

    bool IncrementItemCount(size_t n)   { assert(false); return false; }
    bool DecrementItemCount(size_t n)   { assert(false); return false; }

    void Start()                        { assert(false); }
};

// Enabled
template <typename D>
struct AsyncWorker<D, concurrent_input>
{
    using DataT = tbb::concurrent_bounded_queue<AsyncItem>;

    class WorkerTask : public tbb::task
    {
    public:
        WorkerTask(InputManager<D>& mgr) :
            mgr_( mgr )
        {}

        tbb::task* execute()
        {
            mgr_.processAsyncQueue();
            return nullptr;
        }

    private:
        InputManager<D>& mgr_;
    };

public:
    AsyncWorker(InputManager<D>& mgr) :
        mgr_( mgr )
    {}

    void PushItem(AsyncItem&& item)
    {
        data_.push(std::move(item));
    }

    void PopItem(AsyncItem& item)
    {
        data_.pop(item);
    }

    bool TryPopItem(AsyncItem& item)
    {
        return data_.try_pop(item);
    }

    bool IncrementItemCount(size_t n)
    {
        return count_.fetch_add(n, std::memory_order_relaxed) == 0;
    }

    bool DecrementItemCount(size_t n)
    {
        return count_.fetch_sub(n, std::memory_order_relaxed) == n;
    }

    void Start()
    {
        tbb::task::enqueue(*new(tbb::task::allocate_root()) WorkerTask(mgr_));
    }

private:
    DataT               data_;
    std::atomic<size_t> count_{ 0 };

    InputManager<D>&    mgr_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// InputManager
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class InputManager :
    public IContinuationTarget
{
private:
    // Select between thread-safe and non thread-safe implementations
    using TransactionQueueT = TransactionQueue<D::Policy::input_mode>;
    using QueueEntryT = typename TransactionQueueT::QueueEntry;

    using ContinuationManagerT = ContinuationManager<D::Policy::propagation_mode>;
    using AsyncWorkerT = AsyncWorker<D, D::Policy::input_mode>;

    template <typename, EInputMode>
    friend class AsyncWorker;

public:
    using TurnT = typename D::TurnT;
    using Engine = typename D::Engine;

    InputManager() :
        asyncWorker_(*this)
    {}

    template <typename F>
    void DoTransaction(TransactionFlagsT flags, F&& func)
    {
        // Attempt to add input to another turn.
        // If successful, blocks until other turn is done and returns.
        bool canMerge = (flags & allow_merging) != 0;
        if (canMerge && transactionQueue_.TryMergeSync(std::forward<F>(func)))
            return;

        bool shouldPropagate = false;
        
        QueueEntryT tr( flags );

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

        finalizeSyncTransaction(tr);
    }

    template <typename F>
    void AsyncTransaction(TransactionFlagsT flags, const WaitingStatePtrT& waitingStatePtr,
                          F&& func)
    {
        if (waitingStatePtr != nullptr)
            waitingStatePtr->IncWaitCount();

        asyncWorker_.PushItem(AsyncItem{ flags, waitingStatePtr, std::forward<F>(func) });

        if (asyncWorker_.IncrementItemCount(1))
            asyncWorker_.Start();
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
    virtual void AsyncContinuation(TransactionFlagsT flags,
                                   const WaitingStatePtrT& waitingStatePtr,
                                   TransactionFuncT&& cont) override
    {
        AsyncTransaction(flags, waitingStatePtr, std::move(cont));
    }
    //~IContinuationTarget

    void StoreContinuation(IContinuationTarget& target, TransactionFlagsT flags,
                           TransactionFuncT&& cont)
    {
        continuationManager_.StoreContinuation(target, flags, std::move(cont));
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
        QueueEntryT tr( 0 );

        transactionQueue_.EnterQueue(tr);

        TurnT turn( nextTurnId(), 0 );
        Engine::OnTurnAdmissionStart(turn);
        r.AddInput(std::forward<V>(v));
        tr.RunMergedInputs();
        Engine::OnTurnAdmissionEnd(turn);

        if (r.ApplyInput(&turn))
            Engine::Propagate(turn);

        finalizeSyncTransaction(tr);
    }

    template <typename R, typename F>
    void modifySimpleInput(R& r, const F& func)
    {
        QueueEntryT tr( 0 );

        transactionQueue_.EnterQueue(tr);

        TurnT turn( nextTurnId(), 0 );
        Engine::OnTurnAdmissionStart(turn);
        r.ModifyInput(func);
        Engine::OnTurnAdmissionEnd(turn);

        // Return value, will always be true
        r.ApplyInput(&turn);

        Engine::Propagate(turn);

        finalizeSyncTransaction(tr);
    }

    void finalizeSyncTransaction(QueueEntryT& tr)
    {
        continuationManager_.template DetachQueuedObservers<D>();

        if (continuationManager_.HasContinuations())
        {
            UniqueWaitingState st;
            WaitingStatePtrT p( &st );

            continuationManager_.StartContinuations(p);

            transactionQueue_.ExitQueue(tr);

            p->Wait();
        }
        else
        {
            transactionQueue_.ExitQueue(tr);
        }
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
        AsyncItem   item;

        std::vector<WaitingStatePtrT> waitingStatePtrs;

        bool skipPop = false;

        while (true)
        {
            size_t popCount = 0;

            if (!skipPop)
            {
                // Blocks if queue is empty.
                // This should never happen,
                // and if (maybe due to some memory access internals), only briefly
                asyncWorker_.PopItem(item);
                popCount++;
            }
            else
            {
                skipPop = false;
            }

            // First try to merge to an existing synchronous item in the queue
            bool canMerge = (item.Flags & allow_merging) != 0;
            if (canMerge && transactionQueue_.TryMergeAsync(
                    std::move(item.Func),
                    std::move(item.WaitingStatePtr)))
                continue;

            bool shouldPropagate = false;
            
            QueueEntryT tr( item.Flags );

            // Blocks until turn is at the front of the queue
            transactionQueue_.EnterQueue(tr);

            TurnT turn( nextTurnId(), item.Flags );

            // Phase 1 - Input admission
            ThreadLocalInputState<>::IsTransactionActive = true;

            Engine::OnTurnAdmissionStart(turn);

            // Input of current item
            item.Func();

            // Merged sync inputs that arrived while this item was queued
            tr.RunMergedInputs(); 

            // Save data, item might be re-used for next input
            if (item.WaitingStatePtr != nullptr)
                waitingStatePtrs.push_back(std::move(item.WaitingStatePtr));

            // If the current item supports merging, try to add more mergeable inputs
            // to this turn
            if (canMerge)
            {
                uint extraCount = 0;
                // Todo: Make configurable
                while (extraCount < 1024 && asyncWorker_.TryPopItem(item))
                {
                    ++popCount;

                    bool canMergeNext = (item.Flags & allow_merging) != 0;
                    if (canMergeNext)
                    {
                        item.Func();

                        if (item.WaitingStatePtr != nullptr)
                            waitingStatePtrs.push_back(std::move(item.WaitingStatePtr));

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

            continuationManager_.template DetachQueuedObservers<D>();

            // Has continuations? If so, status ptrs have to be passed on to
            // continuation transactions
            if (continuationManager_.HasContinuations())
            {
                // Merge all waiting state ptrs for this transaction into a single vector
                tr.MoveWaitingStatePtrs(waitingStatePtrs);

                // More than 1 waiting state -> create collection from vector
                if (waitingStatePtrs.size() > 1)
                {
                    WaitingStatePtrT p
                    (
                        SharedWaitingStateCollection::Create(std::move(waitingStatePtrs))
                    );

                    continuationManager_.StartContinuations(p);

                    transactionQueue_.ExitQueue(tr);
                    p->DecWaitCount();
                }
                // Exactly one status ptr -> pass it on directly
                else if (waitingStatePtrs.size() == 1)
                {
                    WaitingStatePtrT p( std::move(waitingStatePtrs[0]) );

                    continuationManager_.StartContinuations(p);

                    transactionQueue_.ExitQueue(tr);
                    p->DecWaitCount();
                }
                // No status ptrs
                else
                {
                    continuationManager_.StartContinuations(nullptr);
                }
            }
            else
            {
                transactionQueue_.ExitQueue(tr);

                for (auto& p : waitingStatePtrs)
                    p->DecWaitCount();
            }

            waitingStatePtrs.clear();

            // Stop this task if the number of items has just been decremented zero.
            // A new task will be started by the thread that increments the item count from zero.
            if (asyncWorker_.DecrementItemCount(popCount))
                break;
        }
    }

    TransactionQueueT       transactionQueue_;
    ContinuationManagerT    continuationManager_;
    AsyncWorkerT            asyncWorker_;

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