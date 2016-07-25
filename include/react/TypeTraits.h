
//          Copyright Sebastian Jeckel 2016.
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
template <typename T>
class Signal;

template <typename T>
class VarSignal;

template <typename T>
class Events;

template <typename T>
class EventSource;

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

template <>
struct IsReactive<Observer> { static const bool value = true; };

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