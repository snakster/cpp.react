
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/detail/Defs.h"

#include "react/TypeTraits.h"
#include "react/detail/ReactiveInput.h"
#include "react/detail/Options.h"

#ifdef REACT_ENABLE_LOGGING
    #include "react/logging/EventLog.h"
    #include "react/logging/EventRecords.h"
#endif //REACT_ENABLE_LOGGING

#include "react/detail/IReactiveEngine.h"
#include "react/engine/ToposortEngine.h"

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward declarations
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class ReactiveLoop;

template <typename D>
class Observer;

template <typename D>
class ScopedObserver;

template <typename D, typename S>
class Signal;

template <typename D, typename S>
class VarSignal;

template <typename D, typename S, typename TOp>
class TempSignal;

template <typename D, typename E>
class Events;

template <typename D, typename E>
class EventSource;

template <typename D, typename E, typename TOp>
class TempEvents;

enum class Token;

using REACT_IMPL::TurnFlagsT;

#ifdef REACT_ENABLE_LOGGING
    using REACT_IMPL::EventLog;
#endif //REACT_ENABLE_LOGGING    

///////////////////////////////////////////////////////////////////////////////////////////////////
/// DomainBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename TPolicy>
class DomainBase
{
public:
    using TurnT = typename TPolicy::Engine::TurnT;

    DomainBase() = delete;

    using Policy = TPolicy;
    using Engine = REACT_IMPL::EngineInterface<D, typename Policy::Engine>;

    ///////////////////////////////////////////////////////////////////////////////////////////////
    /// Domain traits
    ///////////////////////////////////////////////////////////////////////////////////////////////
    static const bool uses_node_update_timer =
        REACT_IMPL::EnableNodeUpdateTimer<typename Policy::Engine>::value;

    static const bool uses_concurrent_input =
        REACT_IMPL::EnableConcurrentInput<typename Policy::Engine>::value;

    static const bool uses_parallel_updating =
        REACT_IMPL::EnableParallelUpdating<typename Policy::Engine>::value;

    ///////////////////////////////////////////////////////////////////////////////////////////////
    /// Aliases for reactives of current domain
    ///////////////////////////////////////////////////////////////////////////////////////////////
    template <typename S>
    using SignalT = Signal<D,S>;

    template <typename S>
    using VarSignalT = VarSignal<D,S>;

    template <typename E = Token>
    using EventsT = Events<D,E>;

    template <typename E = Token>
    using EventSourceT = EventSource<D,E>;

    using ObserverT = Observer<D>;

    using ScopedObserverT = ScopedObserver<D>;

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
    /// MakeVar (higher order)
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
    /// MakeEventSource
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename E>
    static auto MakeEventSource()
        -> EventSourceT<E>
    {
        return REACT::MakeEventSource<D,E>();
    }

    static auto MakeEventSource()
        -> EventSourceT<Token>
    {
        return REACT::MakeEventSource<D>();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoTransaction
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename F>
    static void DoTransaction(F&& func)
    {
        REACT_IMPL::InputManager<D>::DoTransaction(0, std::forward<F>(func));
    }

    template <typename F>
    static void DoTransaction(TurnFlagsT flags, F&& func)
    {
        REACT_IMPL::InputManager<D>::DoTransaction(flags, std::forward<F>(func));
    }

#ifdef REACT_ENABLE_LOGGING
    ///////////////////////////////////////////////////////////////////////////////////////////////
    /// Log
    ///////////////////////////////////////////////////////////////////////////////////////////////
    static EventLog& Log()
    {
        static EventLog instance;
        return instance;
    }
#endif //REACT_ENABLE_LOGGING
};

/******************************************/ REACT_END /******************************************/

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Policy
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename TEngine = ToposortEngine<sequential>
>
struct DomainPolicy
{
    using Engine = TEngine;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Ensure singletons are created immediately after domain declaration (TODO hax)
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class DomainInitializer
{
public:
    DomainInitializer()
    {
#ifdef REACT_ENABLE_LOGGING
        D::Log();
#endif //REACT_ENABLE_LOGGING

        typename D::Engine::Engine();
    }
};

/****************************************/ REACT_IMPL_END /***************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Domain definition macro
///////////////////////////////////////////////////////////////////////////////////////////////////
#define REACTIVE_DOMAIN(name, ...) \
    struct name : public REACT::DomainBase<name, REACT_IMPL::DomainPolicy<__VA_ARGS__ >> {}; \
    REACT_IMPL::DomainInitializer< name > name ## _initializer_;