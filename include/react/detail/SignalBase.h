
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_SIGNALBASE_H_INCLUDED
#define REACT_DETAIL_SIGNALBASE_H_INCLUDED

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
class SignalBase : public CopyableReactive<SignalNode<D,S>>
{
public:
    SignalBase() = default;
    SignalBase(const SignalBase&) = default;
    
    template <typename T>
    SignalBase(T&& t) :
        SignalBase::CopyableReactive( std::forward<T>(t) )
    {}

protected:
    const S& getValue() const
    {
        return this->ptr_->ValueRef();
    }

    template <typename T>
    void setValue(T&& newValue) const
    {
        DomainSpecificInputManager<D>::Instance().AddInput(
            *reinterpret_cast<VarNode<D,S>*>(this->ptr_.get()),
            std::forward<T>(newValue));
    }

    template <typename F>
    void modifyValue(const F& func) const
    {
        DomainSpecificInputManager<D>::Instance().ModifyInput(
            *reinterpret_cast<VarNode<D,S>*>(this->ptr_.get()), func);
    }
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_SIGNALBASE_H_INCLUDED
