
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_REACTIVEBASE_H_INCLUDED
#define REACT_DETAIL_REACTIVEBASE_H_INCLUDED

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
/// ReactiveBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TNode>
class ReactiveBase
{
public:
    using DomainT   = typename TNode::DomainT;
    
    // Default ctor
    ReactiveBase() = default;

    // Copy ctor
    ReactiveBase(const ReactiveBase&) = default;

    // Move ctor (VS2013 doesn't default generate that yet)
    ReactiveBase(ReactiveBase&& other) :
        ptr_( std::move(other.ptr_) )
    {}

    // Explicit node ctor
    explicit ReactiveBase(std::shared_ptr<TNode>&& ptr) :
        ptr_( std::move(ptr) )
    {}

    // Copy assignment
    ReactiveBase& operator=(const ReactiveBase&) = default;

    // Move assignment
    ReactiveBase& operator=(ReactiveBase&& other)
    {
        ptr_.reset(std::move(other));
        return *this;
    }

    bool IsValid() const
    {
        return ptr_ != nullptr;
    }

    void SetWeightHint(WeightHint weight)
    {
        assert(IsValid());
        ptr_->SetWeightHint(weight);
    }

protected:
    std::shared_ptr<TNode>  ptr_;

    template <typename TNode_>
    friend const std::shared_ptr<TNode_>& GetNodePtr(const ReactiveBase<TNode_>& node);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// CopyableReactive
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TNode>
class CopyableReactive : public ReactiveBase<TNode>
{
public:
    CopyableReactive() = default;

    CopyableReactive(const CopyableReactive&) = default;

    CopyableReactive(CopyableReactive&& other) :
        CopyableReactive::ReactiveBase( std::move(other) )
    {}

    explicit CopyableReactive(std::shared_ptr<TNode>&& ptr) :
        CopyableReactive::ReactiveBase( std::move(ptr) )
    {}

    CopyableReactive& operator=(const CopyableReactive&) = default;

    CopyableReactive& operator=(CopyableReactive&& other)
    {
        CopyableReactive::ReactiveBase::operator=(std::move(other));
        return *this;
    }

    bool Equals(const CopyableReactive& other) const
    {
        return this->ptr_ == other.ptr_;
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// UniqueReactiveBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TNode>
class MovableReactive : public ReactiveBase<TNode>
{
public:
    MovableReactive() = default;

    MovableReactive(MovableReactive&& other) :
        MovableReactive::ReactiveBase( std::move(other) )
    {}

    explicit MovableReactive(std::shared_ptr<TNode>&& ptr) :
        MovableReactive::ReactiveBase( std::move(ptr) )
    {}

    MovableReactive& operator=(MovableReactive&& other)
    {
        MovableReactive::ReactiveBase::operator=(std::move(other));
        return *this;
    }

    // Deleted copy ctor and assignment
    MovableReactive(const MovableReactive&) = delete;
    MovableReactive& operator=(const MovableReactive&) = delete;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// GetNodePtr
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TNode>
const std::shared_ptr<TNode>& GetNodePtr(const ReactiveBase<TNode>& node)
{
    return node.ptr_;
}

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_REACTIVEBASE_H_INCLUDED