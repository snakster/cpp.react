
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

#include "tbb/concurrent_vector.h"

#include "react/detail/IReactiveNode.h."
#include "react/detail/Options.h."

/***************************************/ REACT_IMPL_BEGIN /**************************************/

using TurnIdT = uint;
using TurnFlagsT = uint;

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
/// InputManager
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class InputManager
{
public:
    using TurnT = typename D::TurnT;
    using Engine = typename D::Engine;

    template <typename F>
    static void DoTransaction(TurnFlagsT flags, F&& func)
    {
        // Attempt to add input to another turn.
        // If successful, blocks until other turn is done and returns.
        if (Engine::TryMerge(std::forward<F>(func)))
            return;

        bool shouldPropagate = false;

        TurnT turn( nextTurnId(), flags );

        // Phase 1 - Input admission
        transactionState_.Active = true;
        Engine::OnTurnAdmissionStart(turn);
        func();
        Engine::OnTurnAdmissionEnd(turn);
        transactionState_.Active = false;

        // Phase 2 - Apply input node changes
        for (auto* p : transactionState_.Inputs)
            if (p->ApplyInput(&turn))
                shouldPropagate = true;
        transactionState_.Inputs.clear();

        // Phase 3 - Propagate changes
        if (shouldPropagate)
            Engine::OnTurnPropagate(turn);

        Engine::OnTurnEnd(turn);

        postProcessTurn(turn);
    }

    template <typename R, typename V>
    static void AddInput(R& r, V&& v)
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
    static void ModifyInput(R& r, const F& func)
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
    static std::atomic<TurnIdT> nextTurnId_;

    static TurnIdT nextTurnId()
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

    static TransactionState transactionState_;

    // Create a turn with a single input
    template <typename R, typename V>
    static void addSimpleInput(R& r, V&& v)
    {
        TurnT turn( nextTurnId(), 0 );

        Engine::OnTurnAdmissionStart(turn);
        r.AddInput(std::forward<V>(v));
        Engine::OnTurnAdmissionEnd(turn);

        if (r.ApplyInput(&turn))
            Engine::OnTurnPropagate(turn);

        Engine::OnTurnEnd(turn);

        postProcessTurn(turn);
    }

    template <typename R, typename F>
    static void modifySimpleInput(R& r, const F& func)
    {
        TurnT turn( nextTurnId(), 0 );

        Engine::OnTurnAdmissionStart(turn);
        r.ModifyInput(func);
        Engine::OnTurnAdmissionEnd(turn);

        // Return value, will always be true
        r.ApplyInput(&turn);

        Engine::OnTurnPropagate(turn);

        Engine::OnTurnEnd(turn);

        postProcessTurn(turn);
    }

    // This input is part of an active transaction
    template <typename R, typename V>
    static void addTransactionInput(R& r, V&& v)
    {
        r.AddInput(std::forward<V>(v));
        transactionState_.Inputs.push_back(&r);
    }

    template <typename R, typename F>
    static void modifyTransactionInput(R& r, const F& func)
    {
        r.ModifyInput(func);
        transactionState_.Inputs.push_back(&r);
    }

    // Input happened during a turn - buffer in continuation
    template <typename R, typename V>
    static void addContinuationInput(R& r, const V& v)
    {
        // Copy v
        ContinuationHolder<D>::Get()->Add(
            [&r,v] { addTransactionInput(r, std::move(v)); }
        );
    }

    template <typename R, typename F>
    static void modifyContinuationInput(R& r, const F& func)
    {
        // Copy func
        ContinuationHolder<D>::Get()->Add(
            [&r,func] { modifyTransactionInput(r, func); }
        );
    }

    static void postProcessTurn(TurnT& turn)
    {
        turn.template detachObservers<D>();

        // Steal continuation from current turn
        if (! turn.continuation_.IsEmpty())
            processContinuations(std::move(turn.continuation_), 0);
    }

    static void processContinuations(typename TurnT::ContinuationT&& cont, TurnFlagsT flags)
    {
        // No merging for continuations
        flags &= ~enable_input_merging;

        while (true)
        {
            bool shouldPropagate = false;
            TurnT turn( nextTurnId(), flags );

            transactionState_.Active = true;
            Engine::OnTurnAdmissionStart(turn);
            cont.Execute();
            Engine::OnTurnAdmissionEnd(turn);
            transactionState_.Active = false;

            for (auto* p : transactionState_.Inputs)
                if (p->ApplyInput(&turn))
                    shouldPropagate = true;
            transactionState_.Inputs.clear();

            if (shouldPropagate)
                Engine::OnTurnPropagate(turn);

            Engine::OnTurnEnd(turn);

            turn.template detachObservers<D>();

            if (turn.continuation_.IsEmpty())
                break;

            cont = std::move(turn.continuation_);
        }
    }
};

template <typename D>
std::atomic<TurnIdT> InputManager<D>::nextTurnId_( 0 );

template <typename D>
typename InputManager<D>::TransactionState InputManager<D>::transactionState_;

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_REACTIVEINPUT_H_INCLUDED