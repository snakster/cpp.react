
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_EVENTFWD_H_INCLUDED
#define REACT_DETAIL_EVENTFWD_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include "react/TypeTraits.h"

/*****************************************/ REACT_BEGIN /*****************************************/

// Classes
template <typename D, typename E>
class Events;

template <typename D, typename E>
class EventSource;

template <typename D, typename E, typename TOp>
class TempEvents;

enum class Token;

// Functions
template <typename D, typename E>
auto MakeEventSource()
    -> EventSource<D,E>;

template <typename D>
auto MakeEventSource()
    -> EventSource<D,Token>;

/******************************************/ REACT_END /******************************************/

#endif // REACT_DETAIL_EVENTFWD_H_INCLUDED