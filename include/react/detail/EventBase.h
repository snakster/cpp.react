
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/detail/Defs.h"

#include <memory>
#include <utility>

#include "react/detail/ReactiveBase.h"
#include "react/detail/ReactiveInput.h"
#include "react/detail/graph/EventNodes.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventStreamBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename E
>
class EventStreamBase : public ReactiveBase<EventStreamNode<D,E>>
{
public:
    EventStreamBase() = default;

    template <typename T>
    explicit EventStreamBase(T&& ptr) :
        ReactiveBase{ std::forward<T>(ptr) }
    {}

protected:
    template <typename T>
    void emit(T&& e) const
    {
        InputManager<D>::AddInput(
            *std::static_pointer_cast<EventSourceNode<D,E>>(ptr_), std::forward<T>(e));
    }
};

/****************************************/ REACT_IMPL_END /***************************************/
