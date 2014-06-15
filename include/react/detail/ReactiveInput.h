
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_REACTIVEINPUT_H_INCLUDED
#define REACT_DETAIL_REACTIVEINPUT_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <atomic>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <utility>
#include <thread>
#include <vector>

#include "tbb/concurrent_queue.h"
#include "tbb/concurrent_vector.h"

#include "react/detail/Options.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

using TurnIdT = uint;
using TurnFlagsT = uint;

struct IInputNode;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ContinuationInput
///////////////////////////////////////////////////////////////////////////////////////////////////
template <bool is_thread_safe>
class ContinuationInput
{
public:
    ContinuationInput() = default;
    ContinuationInput(const ContinuationInput&) = delete;
    ContinuationInput(ContinuationInput&&);

    bool IsEmpty() const;

    template <typename F>
    void Add(F&& input);

    void Execute();
};

// Not thread-safe
template <>
class ContinuationInput<false>
{
public:
    using InputClosureT = std::function<void()>;
    using InputVectT    = std::vector<InputClosureT>;

    ContinuationInput() = default;
    ContinuationInput(const ContinuationInput&) = delete;

    ContinuationInput(ContinuationInput&& other) :
        bufferedInputs_( std::move(other.bufferedInputs_) )
    {}
    
    ContinuationInput& operator=(ContinuationInput&& other)
    {
        bufferedInputs_ = std::move(other.bufferedInputs_);
        return *this;
    }

    bool IsEmpty() const { return bufferedInputs_.empty(); }

    template <typename F>
    void Add(F&& input)
    {
        bufferedInputs_.push_back(std::forward<F>(input));
    }

    void Execute()
    {
        for (auto& f : bufferedInputs_)
            f();
        bufferedInputs_.clear();
    }

private:
    InputVectT  bufferedInputs_;
};

// Thread-safe
template <>
class ContinuationInput<true>
{
public:
    using InputClosureT = std::function<void()>;
    using InputVectT    = tbb::concurrent_vector<InputClosureT>;

    ContinuationInput() = default;
    ContinuationInput(const ContinuationInput&) = delete;

    ContinuationInput(ContinuationInput&& other) :
        bufferedInputsPtr_( std::move(other.bufferedInputsPtr_) )
    {}
    
    ContinuationInput& operator=(ContinuationInput&& other)
    {
        bufferedInputsPtr_ = std::move(other.bufferedInputsPtr_);
        return *this;
    }

    bool IsEmpty() const { return bufferedInputsPtr_ == nullptr; }

    template <typename F>
    void Add(F&& input)
    {
        // Allocation of concurrent vector is not cheap -> create on demand
        std::call_once(bufferedInputsInit_, [this] {
            bufferedInputsPtr_.reset(new InputVectT());
        });
        bufferedInputsPtr_->push_back(std::forward<F>(input));
    }

    void Execute()
    {
        if (bufferedInputsPtr_ != nullptr)
        {
            for (auto& f : *bufferedInputsPtr_)
                f();
            bufferedInputsPtr_->clear();
        }
    }

private:
    std::once_flag                  bufferedInputsInit_;
    std::unique_ptr<InputVectT>     bufferedInputsPtr_ = nullptr;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ContinuationHolder
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class ContinuationHolder
{
public:
    using TurnT = typename D::TurnT;
    using ContinuationT = ContinuationInput<D::uses_parallel_updating>;

    ContinuationHolder() = delete;

    static void             SetTurn(TurnT& turn)    { ptr_ = &turn.continuation_; }
    static void             Clear()                 { ptr_ = nullptr; }
    static ContinuationT*   Get()                   { return ptr_; }

private:
    static REACT_TLS ContinuationT* ptr_;
};

template <typename D>
REACT_TLS typename ContinuationHolder<D>::ContinuationT* ContinuationHolder<D>::ptr_(nullptr);

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ContinuationInput
///////////////////////////////////////////////////////////////////////////////////////////////////
class TransactionStatus
{
public:
    TransactionStatus() = default;
    TransactionStatus(const TransactionStatus&) = delete;
    TransactionStatus(TransactionStatus&&) = delete;

    inline void Wait()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return !isWaiting_; });
    }

private:
    inline void incWaitCount()
    {
        auto oldVal = waitCount_.fetch_add(1, std::memory_order_relaxed);

        if (oldVal == 0)
        {// mutex_
            std::lock_guard<std::mutex> scopedLock(mutex_);
            isWaiting_ = true;
        }// ~mutex_
    }


    inline void decWaitCount()
    {
        auto oldVal = waitCount_.fetch_sub(1, std::memory_order_relaxed);

        if (oldVal == 1)
        {// mutex_
            std::lock_guard<std::mutex> scopedLock(mutex_);
            isWaiting_ = false;
            condition_.notify_all();
        }// ~mutex_
    }
    

    std::atomic<uint>           waitCount_{ 0 };

    std::condition_variable     condition_;
    std::mutex                  mutex_;
    bool                        isWaiting_{ false };

    template <typename D>
    friend class InputManager;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// InputManager
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class InputManager
{
public:
    using TransactionFuncT = std::function<void()>;
    

    struct AsyncItem
    {
        TurnFlagsT          Flags;
        TransactionStatus*  StatusPtr;
        TransactionFuncT    Func;
    };

    using AsyncQueueT = tbb::concurrent_bounded_queue<AsyncItem>;

    InputManager() :
        asyncWorker_( [this] { processAsyncQueue(); } )
    {
        asyncWorker_.detach();
    }

    AsyncQueueT asyncQueue_;
    std::thread asyncWorker_;

    using TurnT = typename D::TurnT;
    using Engine = typename D::Engine;

    template <typename F>
    void DoTransaction(TurnFlagsT flags, F&& func)
    {
        // Attempt to add input to another turn.
        // If successful, blocks until other turn is done and returns.
        bool canMerge = (flags & enable_input_merging) != 0;
        if (canMerge && Engine::TryMergeInput(std::forward<F>(func), true))
            return;

        bool shouldPropagate = false;

        TurnT turn( nextTurnId(), flags );

        Engine::EnterTurnQueue(turn);

        // Phase 1 - Input admission
        transactionState_.Active = true;
        Engine::OnTurnAdmissionStart(turn);
        func();
        Engine::ApplyMergedInputs(turn);
        Engine::OnTurnAdmissionEnd(turn);
        transactionState_.Active = false;

        // Phase 2 - Apply input node changes
        for (auto* p : transactionState_.Inputs)
            if (p->ApplyInput(&turn))
                shouldPropagate = true;
        transactionState_.Inputs.clear();

        // Phase 3 - Propagate changes
        if (shouldPropagate)
            Engine::Propagate(turn);

        Engine::ExitTurnQueue(turn);

        postProcessTurn(turn);
    }

    template <typename F>
    void AsyncTransaction(TurnFlagsT flags, TransactionStatus* statusPtr, F&& func)
    {
        if (statusPtr != nullptr)
            statusPtr->incWaitCount();

        asyncQueue_.push(AsyncItem{ flags, statusPtr, func } );
    }

    void processAsyncQueue()
    {
        AsyncItem item;

        TransactionStatus* savedStatusPtr = nullptr;
        std::vector<TransactionStatus*> statusPtrStash;

        bool skipPop = false;

        while (true)
        {
            // Blocks if queue is empty
            if (!skipPop)
                asyncQueue_.pop(item);
            else
                skipPop = false;

            // First try to merge to an existing synchronous item in the queue
            bool canMerge = (item.Flags & enable_input_merging) != 0;
            if (canMerge && Engine::TryMergeInput(std::move(item.Func), false))
                return;

            bool shouldPropagate = false;

            TurnT turn( nextTurnId(), item.Flags );

            // Blocks until turn is at the front of the queue
            Engine::EnterTurnQueue(turn);

            // Phase 1 - Input admission
            transactionState_.Active = true;
            Engine::OnTurnAdmissionStart(turn);

            // Input of current item
            item.Func();

            // Merged inputs that arrived while this item was queued
            Engine::ApplyMergedInputs(turn);

            // Save status ptr, because item might be re-used for next input
            savedStatusPtr = item.StatusPtr;

            // If the current item supports merging, try to add more mergeable inputs
            // to this turn
            if (canMerge)
            {
                uint extraCount = 0;
                while (extraCount < 100 && asyncQueue_.try_pop(item))
                {
                    bool canMergeNext = (item.Flags & enable_input_merging) != 0;
                    if (canMergeNext)
                    {
                        item.Func();
                        if (item.StatusPtr != nullptr)
                            statusPtrStash.push_back(item.StatusPtr);

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
            transactionState_.Active = false;

            // Phase 2 - Apply input node changes
            for (auto* p : transactionState_.Inputs)
                if (p->ApplyInput(&turn))
                    shouldPropagate = true;
            transactionState_.Inputs.clear();

            // Phase 3 - Propagate changes
            if (shouldPropagate)
                Engine::Propagate(turn);

            Engine::ExitTurnQueue(turn);

            postProcessTurn(turn);

            // Decrement transaction status counts of processed items
            if (savedStatusPtr != nullptr)
                savedStatusPtr->decWaitCount();

            for (auto* p : statusPtrStash)
                p->decWaitCount();
            statusPtrStash.clear();
        }
    }

    template <typename R, typename V>
    void AddInput(R& r, V&& v)
    {
        if (ContinuationHolder<D>::Get() != nullptr)
        {
            addContinuationInput(r, std::forward<V>(v));
        }
        else if (transactionState_.Active)
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
        if (ContinuationHolder<D>::Get() != nullptr)
        {
            modifyContinuationInput(r, func);
        }
        else if (transactionState_.Active)
        {
            modifyTransactionInput(r, func);
        }
        else
        {
            modifySimpleInput(r, func);
        }
    }

private:
    std::atomic<TurnIdT> nextTurnId_ { 0 };

    TurnIdT nextTurnId()
    {
        auto curId = nextTurnId_.fetch_add(1, std::memory_order_relaxed);

        if (curId == (std::numeric_limits<int>::max)())
            nextTurnId_.fetch_sub((std::numeric_limits<int>::max)());

        return curId;
    }

    struct TransactionState
    {
        bool    Active = false;
        std::vector<IInputNode*>    Inputs;
    };

    TransactionState transactionState_;

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

        postProcessTurn(turn);
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

        postProcessTurn(turn);
    }

    // This input is part of an active transaction
    template <typename R, typename V>
    void addTransactionInput(R& r, V&& v)
    {
        r.AddInput(std::forward<V>(v));
        transactionState_.Inputs.push_back(&r);
    }

    template <typename R, typename F>
    void modifyTransactionInput(R& r, const F& func)
    {
        r.ModifyInput(func);
        transactionState_.Inputs.push_back(&r);
    }

    // Input happened during a turn - buffer in continuation
    template <typename R, typename V>
    void addContinuationInput(R& r, const V& v)
    {
        // Copy v
        ContinuationHolder<D>::Get()->Add(
            [this,&r,v] { addTransactionInput(r, std::move(v)); }
        );
    }

    template <typename R, typename F>
    void modifyContinuationInput(R& r, const F& func)
    {
        // Copy func
        ContinuationHolder<D>::Get()->Add(
            [this,&r,func] { modifyTransactionInput(r, func); }
        );
    }

    void postProcessTurn(TurnT& turn)
    {
        turn.template detachObservers<D>();

        // Steal continuation from current turn
        if (! turn.continuation_.IsEmpty())
            processContinuations(std::move(turn.continuation_), 0);
    }

    void processContinuations(typename TurnT::ContinuationT&& cont, TurnFlagsT flags)
    {
        // No merging for continuations
        flags &= ~enable_input_merging;

        while (true)
        {
            bool shouldPropagate = false;
            TurnT turn( nextTurnId(), flags );

            Engine::EnterTurnQueue(turn);

            transactionState_.Active = true;
            Engine::OnTurnAdmissionStart(turn);
            cont.Execute();
            Engine::ApplyMergedInputs(turn);
            Engine::OnTurnAdmissionEnd(turn);
            transactionState_.Active = false;

            for (auto* p : transactionState_.Inputs)
                if (p->ApplyInput(&turn))
                    shouldPropagate = true;
            transactionState_.Inputs.clear();

            if (shouldPropagate)
                Engine::Propagate(turn);

            Engine::ExitTurnQueue(turn);

            turn.template detachObservers<D>();

            if (turn.continuation_.IsEmpty())
                break;

            cont = std::move(turn.continuation_);
        }
    }
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