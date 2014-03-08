#pragma once

////////////////////////////////////////////////////////////////////////////////////////
#define REACT_BEGIN		namespace react {
#define REACT_END		}
#define REACT			::react

#define REACT_IMPL_BEGIN	REACT_BEGIN	namespace impl {
#define REACT_IMPL_END		REACT_END		}
#define REACT_IMPL			REACT			::impl

////////////////////////////////////////////////////////////////////////////////////////
REACT_BEGIN

typedef unsigned int	uint;
typedef unsigned char	uchar;

REACT_END