
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/detail/Defs.h"

#include <chrono>
#include <cstdint>

/***************************************/ REACT_IMPL_BEGIN /**************************************/

using ObjectId = uintptr_t;

template <typename O>
ObjectId GetObjectId(const O& obj)
{
	return (ObjectId)&obj;
}

using UpdateDurationT = std::chrono::duration<uint, std::micro>;

/****************************************/ REACT_IMPL_END /***************************************/