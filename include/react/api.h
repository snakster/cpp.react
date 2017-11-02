
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_API_H_INCLUDED
#define REACT_API_H_INCLUDED

#pragma once

#include <type_traits>
#include <vector>

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

enum class Token { value };

enum class InPlaceTag
{
    value = 1
};

static constexpr InPlaceTag in_place = InPlaceTag::value;


///////////////////////////////////////////////////////////////////////////////////////////////////
/// API types
///////////////////////////////////////////////////////////////////////////////////////////////////

// Group
class Group;

// Ref
template <typename T>
using Ref = std::reference_wrapper<const T>;

// State
template <typename S>
class State;

template <typename S>
class StateVar;

template <typename S>
class StateSlot;

template <typename S>
class StateLink;

template <typename S>
using StateRef = State<Ref<S>>;

// Event
enum class Token;

template <typename E = Token>
class Event;

template <typename E = Token>
class EventSource;

template <typename E = Token>
class EventSlot;

template <typename E = Token>
using EventValueList = std::vector<E>;

template <typename E = Token>
using EventValueSink = std::back_insert_iterator<std::vector<E>>;

// Observer
class Observer;

template <typename T>
bool HasChanged(const T& a, const T& b)
    { return !(a == b); }

template <typename T>
bool HasChanged(const Ref<T>& a, const Ref<T>& b)
    { return true; }

template <typename T, typename V>
void ListInsert(T& list, V&& value)
    { list.push_back(std::forward<V>(value)); }

template <typename T, typename V>
void MapInsert(T& map, V&& value)
    { map.insert(std::forward<V>(value)); }

/******************************************/ REACT_END /******************************************/

#endif // REACT_API_H_INCLUDED