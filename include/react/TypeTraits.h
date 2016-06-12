
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_TYPETRAITS_H_INCLUDED
#define REACT_TYPETRAITS_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward declarations
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class Signal;

template <typename S>
class VarSignal;

template <typename E>
class Events;

template <typename E>
class EventSource;

template <typename D>
class Observer;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IsSignal
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct IsSignal { static const bool value = false; };

template <typename T>
struct IsSignal<Signal<T>> { static const bool value = true; };

template <typename T>
struct IsSignal<VarSignal<T>> { static const bool value = true; };

template <typename T>
constexpr bool IsSignalType = IsSignal<T>::value;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IsEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct IsEvent { static const bool value = false; };

template <typename T>
struct IsEvent<Events<T>> { static const bool value = true; };

template <typename T>
struct IsEvent<EventSource<T>> { static const bool value = true; };

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IsObserver
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct IsObserver { static const bool value = false; };

template <typename D>
struct IsObserver<Observer<D>> { static const bool value = true; };

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IsObservable
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct IsObservable { static const bool value = false; };

template <typename T>
struct IsObservable<Signal<T>> { static const bool value = true; };

template <typename T>
struct IsObservable<VarSignal<T>> { static const bool value = true; };

template <typename T>
struct IsObservable<Events<T>> { static const bool value = true; };

template <typename T>
struct IsObservable<EventSource<T>> { static const bool value = true; };

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IsReactive
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct IsReactive { static const bool value = false; };

template <typename T>
struct IsReactive<Signal<T>> { static const bool value = true; };

template <typename T>
struct IsReactive<VarSignal<T>> { static const bool value = true; };

template <typename T>
struct IsReactive<Events<T>> { static const bool value = true; };

template <typename T>
struct IsReactive<EventSource<T>> { static const bool value = true; };

template <typename D>
struct IsReactive<Observer<D>> { static const bool value = true; };

///////////////////////////////////////////////////////////////////////////////////////////////////
/// DecayInput
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct DecayInput { using Type = T; };

template <typename T>
struct DecayInput<VarSignal<T>> { using Type = Signal<T>; };

template <typename T>
struct DecayInput<EventSource<T>> { using Type = Events<T>; };

/******************************************/ REACT_END /******************************************/

#endif // REACT_TYPETRAITS_H_INCLUDED