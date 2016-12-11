
//          Copyright Sebastian Jeckel 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_API_H_INCLUDED
#define REACT_API_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"
#include "react/common/Util.h"

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// API constants
///////////////////////////////////////////////////////////////////////////////////////////////////
enum OwnershipPolicy
{
    unique,
    shared
};

enum class WeightHint
{
    automatic,
    light,
    heavy
};

enum class TransactionFlags
{
    none          = 0,
    allow_merging = 1 << 1
};

REACT_DEFINE_BITMASK_OPERATORS(TransactionFlags)

///////////////////////////////////////////////////////////////////////////////////////////////////
/// API types
///////////////////////////////////////////////////////////////////////////////////////////////////

// Groups
template <OwnershipPolicy = unique>
class ReactiveGroup;

// Signals
template <typename S>
class SignalBase;

template <typename S>
class VarSignalBase;

template <typename S>
class SignalSlotBase;

template <typename S>
class SignalLinkBase;

template <typename S, OwnershipPolicy = unique>
class Signal;

template <typename S, OwnershipPolicy = unique>
class VarSignal;

template <typename S, OwnershipPolicy = unique>
class SignalSlot;

template <typename S, OwnershipPolicy = unique>
class SignalLink;

// Events
enum class Token;

template <typename E>
class EventBase;

template <typename E>
class EventSourceBase;

template <typename E>
class EventSlotBase;

template <typename E = Token, OwnershipPolicy = unique>
class Event;

template <typename E = Token, OwnershipPolicy = unique>
class EventSource;

template <typename E, OwnershipPolicy = unique>
class EventSlot;

// Observers
template <OwnershipPolicy = unique>
class Observer;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Traits
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct IsSignal { static const bool value = false; };

template <typename T, OwnershipPolicy ownership_policy>
struct IsSignal<Signal<T, ownership_policy>> { static const bool value = true; };

template <typename T, OwnershipPolicy ownership_policy>
struct IsSignal<VarSignal<T, ownership_policy>> { static const bool value = true; };

template <typename T>
struct IsEvent { static const bool value = false; };

template <typename T, OwnershipPolicy ownership_policy>
struct IsEvent<Event<T, ownership_policy>> { static const bool value = true; };

template <typename T, OwnershipPolicy ownership_policy>
struct IsEvent<EventSource<T, ownership_policy>> { static const bool value = true; };

template <typename T>
struct AsNonInputNode { using type = T; };

template <typename T, OwnershipPolicy ownership_policy>
struct AsNonInputNode<VarSignal<T, ownership_policy>> { using type = Signal<T, ownership_policy>; };

template <typename T, OwnershipPolicy ownership_policy>
struct AsNonInputNode<EventSource<T, ownership_policy>> { using type = Event<T, ownership_policy>; };

/******************************************/ REACT_END /******************************************/

#endif // REACT_TYPETRAITS_H_INCLUDED