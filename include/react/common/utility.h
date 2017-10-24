
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_COMMON_UTIL_H_INCLUDED
#define REACT_COMMON_UTIL_H_INCLUDED

#pragma once

#include "react/detail/defs.h"

#include <tuple>
#include <type_traits>
#include <utility>

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Helper to enable calling a function on each element of an argument pack.
/// We can't do f(args) ...; because ... expands with a comma.
/// But we can do nop_func(f(args) ...);
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename... TArgs>
inline void pass(TArgs&& ...) {}

template <typename T>
bool IsBitmaskSet(T flags, T mask)
{
    return (flags & mask) != (T)0;
}

/****************************************/ REACT_IMPL_END /***************************************/

// Expand args by wrapping them in a dummy function
// Use comma operator to replace potential void return value with 0
#define REACT_EXPAND_PACK(...) REACT_IMPL::pass((__VA_ARGS__ , 0) ...)

#define REACT_DEFINE_BITMASK_OPERATORS(t) \
    inline t operator|(t lhs, t rhs) \
        { return static_cast<t>(static_cast<std::underlying_type<t>::type>(lhs) | static_cast<std::underlying_type<t>::type>(rhs)); } \
    inline t operator&(t lhs, t rhs) \
        { return static_cast<t>(static_cast<std::underlying_type<t>::type>(lhs) & static_cast<std::underlying_type<t>::type>(rhs)); } \
    inline t operator^(t lhs, t rhs) \
        { return static_cast<t>(static_cast<std::underlying_type<t>::type>(lhs) ^ static_cast<std::underlying_type<t>::type>(rhs)); } \
    inline t operator~(t rhs) \
        { return static_cast<t>(~static_cast<std::underlying_type<t>::type>(rhs)); } \
    inline t& operator|=(t& lhs, t rhs) \
        { lhs = static_cast<t>(static_cast<std::underlying_type<t>::type>(lhs) | static_cast<std::underlying_type<t>::type>(rhs)); return lhs; } \
    inline t& operator&=(t& lhs, t rhs) \
        { lhs = static_cast<t>(static_cast<std::underlying_type<t>::type>(lhs) & static_cast<std::underlying_type<t>::type>(rhs)); return lhs; } \
    inline t& operator^=(t& lhs, t rhs) \
        { lhs = static_cast<t>(static_cast<std::underlying_type<t>::type>(lhs) ^ static_cast<std::underlying_type<t>::type>(rhs)); return lhs; }

// Bitmask helper

#endif // REACT_COMMON_UTIL_H_INCLUDED