#pragma once

#include <cstdint>

////////////////////////////////////////////////////////////////////////////////////////
namespace react {

typedef unsigned int	uint;
typedef unsigned char	uchar;

typedef uintptr_t	ObjectId;

template <typename O>
ObjectId GetObjectId(const O& obj)
{
	return (ObjectId)&obj;
}

// ---
}