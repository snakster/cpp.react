
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/detail/Defs.h"

#include <atomic>
#include <functional>
#include <memory>
#include <utility>

#include "tbb/concurrent_vector.h"
#include "tbb/queuing_mutex.h"
#include "tbb/spin_mutex.h"


#include "react/Observer.h"
#include "react/common/Types.h"
#include "react/detail/ReactiveInput.h"
#include "react/detail/Options.h"
#include "react/detail/Traits.h"

#ifdef REACT_ENABLE_LOGGING
    #include "react/detail/logging/EventLog.h"
    #include "react/detail/logging/EventRecords.h"
#endif //REACT_ENABLE_LOGGING

#include "react/engine/TopoSortEngine.h"

/*****************************************/ REACT_BEGIN /*****************************************/

enum class EventToken;

template <typename D>
class ReactiveLoop;

/******************************************/ REACT_END /******************************************/

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Domain
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename TEngine = TopoSortEngine<sequential>
>
struct DomainPolicy
{
    using Engine = TEngine;
};

template <typename D, typename TPolicy>
class DomainBase
{
public:
    using TurnT = typename TPolicy::Engine::TurnT;

    DomainBase() = delete;

    using Policy = TPolicy;
    using Engine = EngineInterface<D, typename Policy::Engine>;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    /// Aliases for reactives of current domain
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename S>
    using SignalT = Signal<D,S>;

    template <typename S>
    using VarSignalT = VarSignal<D,S>;

    template <typename S>
    using RefSignalT = RefSignal<D,S>;

    template <typename S>
    using VarRefSignalT = VarRefSignal<D,S>;

    template <typename E = EventToken>
    using EventsT = Events<D,E>;

    template <typename E = EventToken>
    using EventSourceT = EventSource<D,E>;

    using ObserverT = Observer<D>;

    using ReactiveLoopT = ReactiveLoop<D>;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    /// MakeVar
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template
    <
        typename V,
        typename S = std::decay<V>::type,
        class = std::enable_if<
            !IsSignal<S>::value>::type
    >
    static auto MakeVar(V&& value)
        -> VarSignalT<S>
    {
        return REACT::MakeVar<D>(std::forward<V>(value));
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
            IsSignal<S>::value>::type
    >
    static auto MakeVar(V&& value)
        -> VarSignalT<SignalT<TInner>>
    {
        return REACT::MakeVar<D>(std::forward<V>(value));
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
        -> SignalT<S>
    {
        return REACT::MakeVal<D>(std::forward<V>(value));
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    /// MakeSignal
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template
    <
        typename FIn,
        typename ... TArgs,
        typename F = std::decay<FIn>::type,
        typename S = std::result_of<F(TArgs...)>::type,
        typename TOp = REACT_IMPL::FunctionOp<S,F,REACT_IMPL::SignalNodePtr<D,TArgs> ...>
    >
    static auto MakeSignal(FIn&& func, const SignalT<TArgs>& ... args)
        -> TempSignal<D,S,TOp>
    {
        return REACT::MakeSignal<D>(std::forward<FIn>(func), args ...);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    /// MakeEventSource
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename E>
    static auto MakeEventSource()
        -> EventSourceT<E>
    {
        return REACT::MakeEventSource<D,E>();
    }

    static auto MakeEventSource()
        -> EventSourceT<EventToken>
    {
        return REACT::MakeEventSource<D>();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoTransaction
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename F>
    static void DoTransaction(F&& func)
    {
        InputManager<D>::DoTransaction(0, std::forward<F>(func));
    }

    template <typename F>
    static void DoTransaction(TurnFlagsT flags, F&& func)
    {
        InputManager<D>::DoTransaction(flags, std::forward<F>(func));
    }

#ifdef REACT_ENABLE_LOGGING
    ///////////////////////////////////////////////////////////////////////////////////////////////
    /// Log
    ///////////////////////////////////////////////////////////////////////////////////////////////
    static EventLog& Log()
    {
        static ObserverRegistry<D> instance;
        return instance;
    }
#endif //REACT_ENABLE_LOGGING
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
///// Ensure singletons are created immediately after domain declaration (TODO hax)
/////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class DomainInitializer
{
public:
    DomainInitializer()
    {
        //DomainSpecificData<D>::Observers();

#ifdef REACT_ENABLE_LOGGING
        DomainSpecificData<D>::Log();
#endif //REACT_ENABLE_LOGGING

        typename D::Engine::Engine();
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ContinuationHolder
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class ContinuationHolder
{
public:
    using TurnT = typename D::TurnT;

    ContinuationHolder() = delete;

    static void                 SetTurn(TurnT& turn)    { ptr_ = &turn.continuation_; }
    static void                 Clear()                 { ptr_ = nullptr; }
    static ContinuationInput*   Get()                   { return ptr_; }

private:
    static REACT_TLS ContinuationInput* ptr_;
};

template <typename D>
ContinuationInput* ContinuationHolder<D>::ptr_(nullptr);

/****************************************/ REACT_IMPL_END /***************************************/

#define REACTIVE_DOMAIN(name, ...) \
    struct name : public REACT_IMPL::DomainBase<name, REACT_IMPL::DomainPolicy<__VA_ARGS__ >> {}; \
    REACT_IMPL::DomainInitializer< name > name ## _initializer_;
