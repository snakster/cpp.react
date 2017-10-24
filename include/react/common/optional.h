
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_COMMON_OPTIONAL_H_INCLUDED
#define REACT_COMMON_OPTIONAL_H_INCLUDED

#pragma once

#include "react/detail/defs.h"

#include <chrono>
#include <condition_variable>
#include <mutex>

struct NoValueType
{
    struct Tag {};

    explicit constexpr NoValueType(Tag)
    { }
};

constexpr NoValueType no_value{ NoValueType::Tag{ } };

/***************************************/ REACT_IMPL_BEGIN /**************************************/

template <typename T>
class OptScalarStorage
{
public:
    OptScalarStorage() : hasValue_( false )
    { }

    OptScalarStorage(NoValueType) : hasValue_( false )
    { }

    OptScalarStorage& operator=(NoValueType)
    {
        hasValue_ = false;
        return *this;
    }

    OptScalarStorage(const OptScalarStorage&) = default;

    OptScalarStorage& operator=(const OptScalarStorage&) = default;

    bool HasValue() const
        { return hasValue_; }

    const T& Value() const
        { return value_; }

    T& Value()
        { return value_; }

private:
    T       value_;
    bool    hasValue_;
};

/****************************************/ REACT_IMPL_END /***************************************/

/*****************************************/ REACT_BEGIN /*****************************************/

/******************************************/ REACT_END /******************************************/

#endif // REACT_COMMON_OPTIONAL_H_INCLUDED