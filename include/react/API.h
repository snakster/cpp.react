
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_API_H_INCLUDED
#define REACT_API_H_INCLUDED

#pragma once

#include "react/detail/defs.h"
#include "react/common/utility.h"

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// API constants
///////////////////////////////////////////////////////////////////////////////////////////////////
enum class WeightHint
{
    automatic,
    light,
    heavy
};

enum class TransactionFlags
{
    none            = 0,
    allow_merging   = 1 << 1,
    sync_linked     = 1 << 2
};

REACT_DEFINE_BITMASK_OPERATORS(TransactionFlags)

///////////////////////////////////////////////////////////////////////////////////////////////////
/// API types
///////////////////////////////////////////////////////////////////////////////////////////////////

// Groups
class Group;

// Signals
template <typename S>
class Signal;

template <typename S>
class VarSignal;

template <typename S>
class SignalSlot;

template <typename S>
class SignalLink;

// Events
enum class Token;

template <typename E = Token>
class Event;

template <typename E = Token>
class EventSource;

template <typename E>
class EventSlot;

// Observers
class Observer;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Traits
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct IsSignal { static const bool value = false; };

template <typename T>
struct IsSignal<Signal<T>> { static const bool value = true; };

template <typename T>
struct IsSignal<VarSignal<T>> { static const bool value = true; };

template <typename T>
struct IsEvent { static const bool value = false; };

template <typename T>
struct IsEvent<Event<T>> { static const bool value = true; };

template <typename T>
struct IsEvent<EventSource<T>> { static const bool value = true; };

template <typename T>
struct AsNonInputNode { using type = T; };

template <typename T>
struct AsNonInputNode<VarSignal<T>> { using type = Signal<T>; };

template <typename T>
struct AsNonInputNode<EventSource<T>> { using type = Event<T>; };

/******************************************/ REACT_END /******************************************/

#endif // REACT_TYPETRAITS_H_INCLUDED