
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

// Event
enum class Token;

template <typename E = Token>
class Event;

template <typename E = Token>
class EventSource;

template <typename E>
class EventSlot;

// Observer
class Observer;

/******************************************/ REACT_END /******************************************/

#endif // REACT_TYPETRAITS_H_INCLUDED