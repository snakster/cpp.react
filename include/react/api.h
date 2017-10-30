
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_API_H_INCLUDED
#define REACT_API_H_INCLUDED

#pragma once

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

// State
template <typename S>
class State;

template <typename S>
class StateVar;

template <typename S>
class StateSlot;

template <typename S>
class StateLink;

// Object state
template <typename S>
class ObjectContext;

template <typename T>
class ObjectState;

// Event
enum class Token;

template <typename E = Token>
class Event;

template <typename E = Token>
class EventSource;

template <typename E>
class EventSlot;

template <typename E = Token>
using EventValueList = std::vector<E>;

template <typename E>
using EventValueSink = std::back_insert_iterator<std::vector<E>>;

// Observer
class Observer;

/******************************************/ REACT_END /******************************************/

#endif // REACT_TYPETRAITS_H_INCLUDED