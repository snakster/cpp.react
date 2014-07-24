
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_ENGINEBASE_H_INCLUDED
#define REACT_DETAIL_ENGINEBASE_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "tbb/queuing_mutex.h"

#include "react/common/Concurrency.h"
#include "react/common/Types.h"
#include "react/detail/ReactiveInput.h"
#include "react/detail/IReactiveNode.h"
#include "react/detail/IReactiveEngine.h"
#include "react/detail/ObserverBase.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// TurnBase
///////////////////////////////////////////////////////////////////////////////////////////////////
class TurnBase
{
public:
    inline TurnBase(TurnIdT id, TransactionFlagsT flags) :
        id_( id )
    {}

    inline TurnIdT Id() const { return id_; }

private:
    TurnIdT    id_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_ENGINEBASE_H_INCLUDED