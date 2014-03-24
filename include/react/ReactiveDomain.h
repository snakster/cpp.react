
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/Defs.h"

#include <atomic>
#include <functional>
#include <memory>
#include <utility>

#include "tbb/concurrent_vector.h"
#include "tbb/queuing_mutex.h"
#include "tbb/spin_mutex.h"

#include "react/Observer.h"
#include "react/Options.h"
#include "react/Traits.h"

#include "react/common/ContinuationInput.h"
#include "react/common/Types.h"

#include "react/logging/EventLog.h"
#include "react/logging/EventRecords.h"

#include "react/propagation/TopoSortEngine.h"

/*****************************************/ REACT_BEGIN /*****************************************/

enum class EventToken;

template
<
    typename D,
    typename F,
    typename ... TArgs
>
auto MakeSignal(F&& func, const RSignal<D,TArgs>& ... args)
    -> RSignal<D, typename std::result_of<F(TArgs...)>::type>;

template <typename D>
class RReactiveLoop;

/******************************************/ REACT_END /******************************************/

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Domain
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename TEngine = TopoSortEngine<sequential>,
    typename TLog = NullLog
>
struct DomainPolicy
{
    using Engine    = TEngine;
    using Log        = TLog;
};

template <typename D, typename TPolicy>
class DomainBase
{
public:
    using TurnT = typename TPolicy::Engine::TurnInterface;

    DomainBase() = delete;

    using Policy = TPolicy;
    using Engine = EngineInterface<D, typename Policy::Engine>;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    /// Aliases for reactives of current domain
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename S>
    using Signal = RSignal<D,S>;

    template <typename S>
    using VarSignal = RVarSignal<D,S>;

    template <typename S>
    using RefSignal = RRefSignal<D,S>;

    template <typename S>
    using VarRefSignal = RVarRefSignal<D,S>;

    template <typename E = EventToken>
    using Events = REvents<D,E>;

    template <typename E = EventToken>
    using EventSource = REventSource<D,E>;

    using Observer = RObserver<D>;

    using ReactiveLoop = RReactiveLoop<D>;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    /// ObserverRegistry
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    static ObserverRegistry<D>& Observers()
    {
        static ObserverRegistry<D> registry;
        return registry;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    /// Log
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    static typename Policy::Log& Log()
    {
        static Policy::Log log;
        return log;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    /// MakeVar
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template
    <
        typename V,
        typename S = std::decay<V>::type,
        class = std::enable_if<
            !IsSignal<D,S>::value>::type
    >
    static auto MakeVar(V&& value)
        -> VarSignal<S>
    {
        return react::MakeVar<D>(std::forward<V>(value));
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // MakeVar (higher order signal)
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    template
    <
        typename V,
        typename S = std::decay<V>::type,
        typename TInner = S::ValueT,
        class = std::enable_if<
            IsSignal<D,S>::value>::type
    >
    static auto MakeVar(V&& value)
        -> VarSignal<Signal<TInner>>
    {
        return react::MakeVar<D>(std::forward<V>(value));
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    /// MakeVal
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template
    <
        typename V,
        typename S = std::decay<V>::type
    >
    static auto MakeVal(V&& value)
        -> Signal<S>
    {
        return react::MakeVal<D>(std::forward<V>(value));
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    /// MakeSignal
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template
    <
        typename F,
        typename ... TArgs
    >
    static auto MakeSignal(F&& func, const Signal<TArgs>& ... args)
        -> Signal<typename std::result_of<F(TArgs...)>::type>
    {
        using S = typename std::result_of<F(TArgs...)>::type;

        return react::MakeSignal<D>(std::forward<F>(func), args ...);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    /// MakeEventSource
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename E>
    static auto MakeEventSource()
        -> EventSource<E>
    {
        return react::MakeEventSource<D,E>();
    }

    static auto MakeEventSource()
        -> EventSource<EventToken>
    {
        return react::MakeEventSource<D>();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoTransaction
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename F>
    static void DoTransaction(F&& func)
    {
        DoTransaction(turnFlags_, std::forward<F>(func));
    }

    template <typename F>
    static void DoTransaction(TurnFlagsT flags, F&& func)
    {
        // Attempt to add input to another turn.
        // If successful, blocks until other turn is done and returns.
        if (Engine::TryMerge(std::forward<F>(func)))
            return;

        bool shouldPropagate = false;

        auto turn = makeTurn(flags);

        // Phase 1 - Input admission
        transactionState_.Active = true;
        Engine::OnTurnAdmissionStart(turn);
        func();
        Engine::OnTurnAdmissionEnd(turn);
        transactionState_.Active = false;

        // Phase 2 - Apply input node changes
        for (auto* p : transactionState_.Inputs)
            if (p->Tick(&turn) == ETickResult::pulsed)
                shouldPropagate = true;
        transactionState_.Inputs.clear();

        // Phase 3 - Propagate changes
        if (shouldPropagate)
            Engine::OnTurnPropagate(turn);

        Engine::OnTurnEnd(turn);

        postProcessTurn(turn);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddInput
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename R, typename V>
    static void AddInput(R&& r, V&& v)
    {
        if (! ContinuationHolder_::IsNull())
        {
            addContinuationInput(std::forward<R>(r), std::forward<V>(v));
        }
        else if (transactionState_.Active)
        {
            addTransactionInput(std::forward<R>(r), std::forward<V>(v));
        }
        else
        {
            addSimpleInput(std::forward<R>(r), std::forward<V>(v));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    /// Set/Clear continuation
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    static void SetCurrentContinuation(TurnT& turn)
    {
        ContinuationHolder_::Set(&turn.continuation_);
    }

    static void ClearCurrentContinuation()
    {
        ContinuationHolder_::Reset();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    /// Options
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename Opt>
    static void Set(uint v)        { static_assert(false, "Set option not implemented."); }

    template <typename Opt>
    static bool IsSet(uint v)    { static_assert(false, "IsSet option not implemented."); }

    template <typename Opt>
    static void Unset(uint v)    { static_assert(false, "Unset option not implemented."); }    

    template <typename Opt>
    static void Reset()            { static_assert(false, "Reset option not implemented."); }

    template <> static void Set<ETurnFlags>(uint v)        { turnFlags_ |= v; }
    template <> static bool IsSet<ETurnFlags>(uint v)    { return (turnFlags_ & v) != 0; }
    template <> static void Unset<ETurnFlags>(uint v)    { turnFlags_ &= ~v; }
    template <> static void Reset<ETurnFlags>()            { turnFlags_ = 0; }

private:

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    /// Transaction input continuation
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct ContinuationHolder_ : public ThreadLocalStaticPtr<ContinuationInput> {};

    static __declspec(thread) TurnFlagsT turnFlags_;

    static std::atomic<TurnIdT> nextTurnId_;

    static TurnIdT nextTurnId()
    {
        auto curId = nextTurnId_.fetch_add(1, std::memory_order_relaxed);

        if (curId == INT_MAX)
            nextTurnId_.fetch_sub(INT_MAX);

        return curId;
    }

    struct TransactionState
    {
        bool    Active = false;
        std::vector<IReactiveNode*>    Inputs;
    };

    static TransactionState transactionState_;

    static TurnT makeTurn(TurnFlagsT flags)
    {
        return TurnT(nextTurnId(), flags);
    }

    // Create a turn with a single input
    template <typename R, typename V>
    static void addSimpleInput(R&& r, V&& v)
    {
        auto turn = makeTurn(0);

        Engine::OnTurnAdmissionStart(turn);
        r.AddInput(std::forward<V>(v));
        Engine::OnTurnAdmissionEnd(turn);

        if (r.Tick(&turn) == ETickResult::pulsed)
            Engine::OnTurnPropagate(turn);

        Engine::OnTurnEnd(turn);

        postProcessTurn(turn);
    }

    // This input is part of an active transaction
    template <typename R, typename V>
    static void addTransactionInput(R&& r, V&& v)
    {
        r.AddInput(std::forward<V>(v));
        transactionState_.Inputs.push_back(&r);
    }

    // Input happened during a turn - buffer in continuation
    template <typename R, typename V>
    static void addContinuationInput(R&& r, V&& v)
    {
        // Copy v
        ContinuationHolder_::Get()->Add(
            [&r,v] { addTransactionInput(r, std::move(v)); }
        );
    }

    static void postProcessTurn(TurnT& turn)
    {
        turn.detachObservers(Observers());

        // Steal continuation from current turn
        if (! turn.continuation_.IsEmpty())
            processContinuations(std::move(turn.continuation_), 0);
    }

    static void processContinuations(ContinuationInput&& cont, TurnFlagsT flags)
    {
        // No merging for continuations
        flags &= ~enable_input_merging;

        while (true)
        {
            bool shouldPropagate = false;
            auto turn = makeTurn(flags);

            transactionState_.Active = true;
            Engine::OnTurnAdmissionStart(turn);
            cont.Execute();
            Engine::OnTurnAdmissionEnd(turn);
            transactionState_.Active = false;

            for (auto* p : transactionState_.Inputs)
                if (p->Tick(&turn) == ETickResult::pulsed)
                    shouldPropagate = true;
            transactionState_.Inputs.clear();

            if (shouldPropagate)
                Engine::OnTurnPropagate(turn);

            Engine::OnTurnEnd(turn);

            turn.detachObservers(Observers());

            if (turn.continuation_.IsEmpty())
                break;

            cont = std::move(turn.continuation_);
        }
    }
};

template <typename D, typename TPolicy>
std::atomic<TurnIdT> DomainBase<D,TPolicy>::nextTurnId_( 0 );

template <typename D, typename TPolicy>
TurnFlagsT DomainBase<D,TPolicy>::turnFlags_( 0 );

template <typename D, typename TPolicy>
typename DomainBase<D,TPolicy>::TransactionState DomainBase<D,TPolicy>::transactionState_;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Ensure singletons are created immediately after domain declaration (TODO hax)
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class DomainInitializer
{
public:
    DomainInitializer()
    {
        D::Log();
        typename D::Engine::Engine();
    }
};

/****************************************/ REACT_IMPL_END /***************************************/

#define REACTIVE_DOMAIN(name, ...) \
    struct name : public REACT_IMPL::DomainBase<name, REACT_IMPL::DomainPolicy<__VA_ARGS__ >> {}; \
    REACT_IMPL::DomainInitializer< name > name ## _initializer_;
