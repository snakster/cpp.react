
//          Copyright Sebastian Jeckel 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_GROUP_H_INCLUDED
#define REACT_GROUP_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <memory>
#include <utility>

#include "react/API.h"

#include "react/detail/IReactiveGraph.h"
#include "react/detail/graph/PropagationST.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// GroupInternals
///////////////////////////////////////////////////////////////////////////////////////////////////
class GroupInternals
{
    using GraphType = REACT_IMPL::ReactiveGraph;

public:
    GroupInternals() :
        graphPtr_( std::make_shared<GraphType>() )
        {  }

    GroupInternals(const GroupInternals&) = default;
    GroupInternals& operator=(const GroupInternals&) = default;

    GroupInternals(GroupInternals&&) = default;
    GroupInternals& operator=(GroupInternals&&) = default;

    auto GetGraphPtr() -> std::shared_ptr<GraphType>&
        { return graphPtr_; }

    auto GetGraphPtr() const -> const std::shared_ptr<GraphType>&
        { return graphPtr_; }

private:
    std::shared_ptr<GraphType> graphPtr_;
};

/****************************************/ REACT_IMPL_END /***************************************/

/*****************************************/ REACT_BEGIN /*****************************************/

#if 0

///////////////////////////////////////////////////////////////////////////////////////////////////
/// TransactionStatus
///////////////////////////////////////////////////////////////////////////////////////////////////
class TransactionStatus
{
    using StateT = REACT_IMPL::SharedWaitingState;
    using PtrT = REACT_IMPL::WaitingStatePtrT;

public:
    // Default ctor
    TransactionStatus() :
        statePtr_( StateT::Create() )
        { }

    // Move ctor
    TransactionStatus(TransactionStatus&& other) :
        statePtr_( std::move(other.statePtr_) )
    {
        other.statePtr_ = StateT::Create();
    }

    // Move assignment
    TransactionStatus& operator=(TransactionStatus&& other)
    {
        if (this != &other)
        {
            statePtr_ = std::move(other.statePtr_);
            other.statePtr_ = StateT::Create();
        }
        return *this;
    }

    // Deleted copy ctor & assignment
    TransactionStatus(const TransactionStatus&) = delete;
    TransactionStatus& operator=(const TransactionStatus&) = delete;

    void Wait()
    {
        assert(statePtr_.Get() != nullptr);
        statePtr_->Wait();
    }

private:
    PtrT statePtr_;

    template <typename D, typename F>
    friend void AsyncTransaction(TransactionStatus& status, F&& func);

    template <typename D, typename F>
    friend void AsyncTransaction(TransactionFlagsT flags, TransactionStatus& status, F&& func);
};

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Group
///////////////////////////////////////////////////////////////////////////////////////////////////
class Group : protected REACT_IMPL::GroupInternals
{
public:
    Group() = default;

    Group(const Group&) = default;
    Group& operator=(const Group&) = default;

    Group(Group&&) = default;
    Group& operator=(Group&&) = default;

    template <typename F>
    void DoTransaction(F&& func)
        { GetGraphPtr()->DoTransaction(std::forward<F>(func)); }

    template <typename F>
    void EnqueueTransaction(F&& func)
        { EnqueueTransaction(TransactionFlags::none, std::forward<F>(func)); }

    template <typename F>
    void EnqueueTransaction(TransactionFlags flags, F&& func)
        { GetGraphPtr()->EnqueueTransaction(flags, std::forward<F>(func)); }

    friend bool operator==(const Group& a, const Group& b)
        { return a.GetGraphPtr() == b.GetGraphPtr(); }

    friend bool operator!=(const Group& a, const Group& b)
        { return !(a == b); }

    friend auto GetInternals(Group& g) -> REACT_IMPL::GroupInternals&
        { return g; }

    friend auto GetInternals(const Group& g) -> const REACT_IMPL::GroupInternals&
        { return g; }
};

/******************************************/ REACT_END /******************************************/

#endif // REACT_GROUP_H_INCLUDED