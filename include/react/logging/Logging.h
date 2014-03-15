
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/Defs.h"

#include <iostream>

/*********************************/ REACT_IMPL_BEGIN /*********************************/

////////////////////////////////////////////////////////////////////////////////////////
/// IEventRecord
////////////////////////////////////////////////////////////////////////////////////////
struct IEventRecord
{
	virtual ~IEventRecord() {}

	virtual const char*	EventId() const = 0;
	virtual void		Serialize(std::ostream& out) const = 0;
};

////////////////////////////////////////////////////////////////////////////////////////
/// NullLog
////////////////////////////////////////////////////////////////////////////////////////
class NullLog
{
public:
	template <typename TEventRecord, typename ... TArgs>
	void Append(TArgs ... args)
	{
		// Do nothing
	}
};

/**********************************/ REACT_IMPL_END /**********************************/

/***********************************/ REACT_BEGIN /************************************/

using NullLog = REACT_IMPL::NullLog;

/************************************/ REACT_END /*************************************/