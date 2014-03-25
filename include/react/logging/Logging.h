
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/Defs.h"

#include <iostream>

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IEventRecord
///////////////////////////////////////////////////////////////////////////////////////////////////
struct IEventRecord
{
    virtual ~IEventRecord() {}

    virtual const char* EventId() const = 0;
    virtual void        Serialize(std::ostream& out) const = 0;
};

/****************************************/ REACT_IMPL_END /***************************************/
