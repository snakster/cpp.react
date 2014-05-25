
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/detail/Defs.h"

#include <utility>

#include "react/detail/ReactiveBase.h"
#include "react/detail/ReactiveInput.h"
#include "react/detail/graph/SignalNodes.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SignalBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S
>
class SignalBase : public ReactiveBase<SignalNode<D,S>>
{
public:
    SignalBase() = default;
    SignalBase(const SignalBase&) = default;
    
    template <typename T>
    explicit SignalBase(T&& ptr) :
        ReactiveBase{ std::forward<T>(ptr) }
    {}

protected:
    const S& getValue() const
    {
        return ptr_->ValueRef();
    }

    template <typename T>
    void setValue(T&& newValue) const
    {
        InputManager<D>::AddInput(
            *reinterpret_cast<VarNode<D,S>*>(ptr_.get()),
            std::forward<T>(newValue));
    }

    template <typename F>
    void modifyValue(const F& func) const
    {
        InputManager<D>::ModifyInput(
            *reinterpret_cast<VarNode<D,S>*>(ptr_.get()), func);
    }
};

/****************************************/ REACT_IMPL_END /***************************************/
