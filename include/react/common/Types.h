#pragma once

#include "react/Defs.h"

#include <cstdint>

REACT_BEGIN_

typedef uintptr_t	ObjectId;

template <typename O>
ObjectId GetObjectId(const O& obj)
{
	return (ObjectId)&obj;
}

REACT_END_