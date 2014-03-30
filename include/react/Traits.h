
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/Defs.h"

/*****************************************/ REACT_BEGIN /*****************************************/

template <typename D, typename S>
class Signal;

template <typename D, typename S>
class VarSignal;

template <typename D, typename S, typename TOp>
class TempSignal;

template <typename D, typename S>
using RefSignal = Signal<D,std::reference_wrapper<S>>;

template <typename D, typename S>
using VarRefSignal = VarSignal<D,std::reference_wrapper<S>>;

template <typename D, typename E>
class Events;

template <typename D, typename E>
class EventSource;

enum class EventToken;

template
<
    typename D,
    typename F,
    typename ... TArgs,
    typename S = std::result_of<F(TArgs...)>::type,
    typename TOp = REACT_IMPL::FunctionOp<S,F, REACT_IMPL::SignalNodePtr<D,TArgs> ...>
>
auto MakeSignal(F&& func, const Signal<D,TArgs>& ... args)
    -> TempSignal<D,S,TOp>;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IsSignal
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct IsSignal { static const bool value = false; };

template <typename D, typename T>
struct IsSignal<Signal<D,T>> { static const bool value = true; };

template <typename D, typename T>
struct IsSignal<VarSignal<D,T>> { static const bool value = true; };

template <typename D, typename T, typename TOp>
struct IsSignal<TempSignal<D,T,TOp>> { static const bool value = true; };

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IsEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename T>
struct IsEvent { static const bool value = false; };

template <typename D, typename T>
struct IsEvent<D, Events<D,T>> { static const bool value = true; };

template <typename D, typename T>
struct IsEvent<D, EventSource<D,T>> { static const bool value = true; };

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IsReactive
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename T>
struct IsReactive { static const bool value = false; };

template <typename D, typename T>
struct IsReactive<D, Signal<D,T>> { static const bool value = true; };

template <typename D, typename T>
struct IsReactive<D, VarSignal<D,T>> { static const bool value = true; };

template <typename D, typename T>
struct IsReactive<D, Events<D,T>> { static const bool value = true; };

template <typename D, typename T>
struct IsReactive<D, EventSource<D,T>> { static const bool value = true; };

///////////////////////////////////////////////////////////////////////////////////////////////////
/// RemoveInput
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename T>
struct RemoveInput { using Type = T; };

template <typename D, typename T>
struct RemoveInput<D, VarSignal<D,T>> { using Type = Signal<D,T>; };

template <typename D, typename T>
struct RemoveInput<D, EventSource<D,T>> { using Type = Events<D,T>; };

/******************************************/ REACT_END /******************************************/