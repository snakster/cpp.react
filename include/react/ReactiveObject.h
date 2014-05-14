
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/detail/Defs.h"

#include <utility>
#include <type_traits>

#include "react/TypeTraits.h"
#include "react/common/Util.h"


/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward declarations
///////////////////////////////////////////////////////////////////////////////////////////////////
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

template <typename D>
class ReactiveLoop;

template <typename D>
class Observer;

template <typename D>
class ScopedObserver;

enum class EventToken;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ReactiveObject
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class ReactiveObject
{
public:
    ///////////////////////////////////////////////////////////////////////////////////////////////
    /// Aliases
    ///////////////////////////////////////////////////////////////////////////////////////////////
    using DomainT = D;

    template <typename S>
    using SignalT = Signal<D,S>;

    template <typename S>
    using VarSignalT = VarSignal<D,S>;

    template <typename E = EventToken>
    using EventsT = Events<D,E>;

    template <typename E = EventToken>
    using EventSourceT = EventSource<D,E>;

    using ObserverT = Observer<D>;

    using ScopedObserverT = ScopedObserver<D>;

    using ReactiveLoopT = ReactiveLoop<D>;

    ///////////////////////////////////////////////////////////////////////////////////////////////
    /// MakeVar
    ///////////////////////////////////////////////////////////////////////////////////////////////
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

    template
    <
        typename S
    >
    static auto MakeVar(std::reference_wrapper<S> value)
        -> VarSignalT<S&>
    {
        return REACT::MakeVar<D>(value);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // MakeVar (higher order)
    ///////////////////////////////////////////////////////////////////////////////////////////////
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

    ///////////////////////////////////////////////////////////////////////////////////////////////
    /// MakeSignal
    ///////////////////////////////////////////////////////////////////////////////////////////////
    template
    <
        typename FIn,
        typename ... TArgs,
        typename F = std::decay<FIn>::type,
        typename S = std::result_of<F(TArgs...)>::type
    >
    static auto MakeSignal(FIn&& func, const SignalT<TArgs>& ... args)
        -> SignalT<S>
    {
        return REACT::MakeSignal<D>(std::forward<FIn>(func), args ...);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    /// MakeEventSource
    ///////////////////////////////////////////////////////////////////////////////////////////////
    template <typename E>
    static auto MakeEventSource()
        -> EventSourceT<E>
    {
        return REACT::MakeEventSource<D,E>();
    }

    static auto MakeEventSource()
        -> EventSource<D,EventToken>
    {
        return REACT::MakeEventSource<D>();
    }
};

/******************************************/ REACT_END /******************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Flatten macros
///////////////////////////////////////////////////////////////////////////////////////////////////
// Note: Using static_cast rather than -> return type, because when using lambda for inline
// class initialization, decltype did not recognize the parameter r
#define REACTIVE_REF(obj, name)                                                             \
    Flatten(                                                                                \
        MakeSignal(                                                                         \
            [] (const REACT_IMPL::Identity<decltype(obj)>::Type::ValueT& r)                 \
            {                                                                               \
                using T = decltype(r.name);                                                 \
                return static_cast<RemoveInput<typename T::DomainT, T>::Type>(r.name);      \
            },                                                                              \
            obj))

#define REACTIVE_PTR(obj, name)                                                             \
    Flatten(                                                                                \
        MakeSignal(                                                                         \
            [] (REACT_IMPL::Identity<decltype(obj)>::Type::ValueT r)                        \
            {                                                                               \
                REACT_ASSERT(r != nullptr);                                                 \
                using T = decltype(r->name);                                                \
                return static_cast<RemoveInput<typename T::DomainT, T>::Type>(r->name);     \
            },                                                                              \
            obj))
