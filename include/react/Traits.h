
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/Defs.h"

/***********************************/ REACT_BEGIN /************************************/

template <typename D, typename S>
class RSignal;

template <typename D, typename S>
class RVarSignal;

template <typename D, typename S>
using RRefSignal = RSignal<D,std::reference_wrapper<S>>;

template <typename D, typename S>
using RVarRefSignal = RVarSignal<D,std::reference_wrapper<S>>;

template <typename D, typename E>
class REvents;

template <typename D, typename E>
class REventSource;

enum class EventToken;

template
<
	typename D,
	typename F,
	typename ... TArgs
>
auto MakeSignal(F&& func, const RSignal<D,TArgs>& ... args)
	-> RSignal<D, typename std::result_of<F(TArgs...)>::type>;

////////////////////////////////////////////////////////////////////////////////////////
/// IsSignalT
////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename T>
struct IsSignalT { static const bool value = false; };

template <typename D, typename T>
struct IsSignalT<D, RSignal<D,T>> { static const bool value = true; };

template <typename D, typename T>
struct IsSignalT<D, RVarSignal<D,T>> { static const bool value = true; };

////////////////////////////////////////////////////////////////////////////////////////
/// IsEventT
////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename T>
struct IsEventT { static const bool value = false; };

template <typename D, typename T>
struct IsEventT<D, REvents<D,T>> { static const bool value = true; };

template <typename D, typename T>
struct IsEventT<D, REventSource<D,T>> { static const bool value = true; };

/************************************/ REACT_END /*************************************/