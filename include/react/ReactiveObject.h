
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/detail/Defs.h"

#include <utility>
#include <type_traits>

#include "react/detail/ReactiveBase.h"

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ReactiveObject
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class ReactiveObject
{
public:
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    /// Aliases
    ///////////////////////////////////////////////////////////////////////////////////////////////////
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

    template
    <
        typename S
    >
    static auto MakeVar(std::reference_wrapper<S> value)
        -> VarSignalT<S&>
    {
        return REACT::MakeVar<D>(value);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // MakeVar (higher order)
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
    /// MakeSignal
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template
    <
        typename F,
        typename ... TArgs
    >
    static auto MakeSignal(F&& func, const SignalT<TArgs>& ... args)
        -> SignalT<typename std::result_of<F(TArgs...)>::type>
    {
        using S = typename std::result_of<F(TArgs...)>::type;

        return REACT::MakeSignal<D>(std::forward<F>(func), args ...);
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

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    /// Flatten macros
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // Todo: Add safety wrapper + static assert to check for this for ReactiveObject
    // Note: Using static_cast rather than -> return type, because when using lambda for inline class
    // initialization, decltype did not recognize the parameter r
    #define REACTIVE_REF(obj, name)                                                             \
        Flatten(                                                                                \
            MakeSignal(                                                                         \
                [] (REACT_IMPL::Identity<decltype(obj)>::Type::ValueT::type r)                  \
                {                                                                               \
                    return static_cast<RemoveInput<DomainT, decltype(r.name)>::Type>(r.name);   \
                },                                                                              \
                obj))

    #define REACTIVE_PTR(obj, name)                                                             \
        Flatten(                                                                                \
            MakeSignal(                                                                         \
                [] (REACT_IMPL::Identity<decltype(obj)>::Type::ValueT r)                        \
                {                                                                               \
                    REACT_ASSERT(r != nullptr);                                                 \
                    return static_cast<RemoveInput<DomainT, decltype(r->name)>::Type>(r->name); \
                },                                                                              \
                obj))
};

/******************************************/ REACT_END /******************************************/
