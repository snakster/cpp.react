
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_SIGNALFWD_H_INCLUDED
#define REACT_DETAIL_SIGNALFWD_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <functional>
#include <type_traits>

#include "react/TypeTraits.h"

/*****************************************/ REACT_BEGIN /*****************************************/

// Classes
template <typename D, typename S>
class Signal;

template <typename D, typename S>
class VarSignal;

template <typename D, typename S, typename TOp>
class TempSignal;

// Functions
template
<
    typename D,
    typename V,
    typename S = typename std::decay<V>::type,
    class = typename std::enable_if<
        ! IsReactive<S>::value>::type
>
auto MakeVar(V&& value)
    -> VarSignal<D,S>;

template
<
    typename D,
    typename S
>
auto MakeVar(std::reference_wrapper<S> value)
    -> VarSignal<D,S&>;

/******************************************/ REACT_END /******************************************/

#endif // REACT_DETAIL_SIGNALFWD_H_INCLUDED