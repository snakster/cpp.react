
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/detail/Defs.h"

#include <memory>
#include <utility>

#include "react/detail/graph/GraphBase.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

template <typename L, typename R>
bool Equals(const L& lhs, const R& rhs)
{
    return lhs == rhs;
}

template <typename L, typename R>
bool Equals(const std::reference_wrapper<L>& lhs, const std::reference_wrapper<R>& rhs)
{
    return lhs.get() == rhs.get();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Reactive
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TNode>
class ReactiveBase
{
public:
    using DomainT   = typename TNode::DomainT;
    using NodePtrT  = std::shared_ptr<TNode>;

    ReactiveBase() = default;
    ReactiveBase(const ReactiveBase&) = default;

    ReactiveBase(ReactiveBase&& other) :
        ptr_{ std::move(other.ptr_) }
    {}

    explicit ReactiveBase(const NodePtrT& ptr) :
        ptr_{ ptr }
    {}

    explicit ReactiveBase(NodePtrT&& ptr) :
        ptr_{ std::move(ptr) }
    {}

    const std::shared_ptr<TNode>& NodePtr() const
    {
        return ptr_;
    }

    bool Equals(const ReactiveBase& other) const
    {
        return ptr_ == other.ptr_;
    }

    bool IsValid() const
    {
        return ptr_ != nullptr;
    }

protected:
    std::shared_ptr<TNode>  ptr_;
};

/****************************************/ REACT_IMPL_END /***************************************/
