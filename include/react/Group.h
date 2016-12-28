
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

struct PrivateGroupInterface;
struct CtorTag { };

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
    inline TransactionStatus() :
        statePtr_( StateT::Create() )
    {}

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

    inline void Wait()
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
/// GroupBase
///////////////////////////////////////////////////////////////////////////////////////////////////
class Group
{
    using GraphType = REACT_IMPL::ReactiveGraph;

public:
    Group() :
        graphPtr_( std::make_shared<GraphType>() )
        {  }

    Group(const Group&) = default;
    Group& operator=(const Group&) = default;

    Group(Group&& other) = default;
    Group& operator=(Group&& other) = default;

    template <typename F>
    void DoTransaction(F&& func)
        { graphPtr_->DoTransaction(std::forward<F>(func)); }

    template <typename F>
    void EnqueueTransaction(F&& func)
        { EnqueueTransaction(TransactionFlags::none, std::forward<F>(func)); }

    template <typename F>
    void EnqueueTransaction(TransactionFlags flags, F&& func)
        { graphPtr_->EnqueueTransaction(flags, std::forward<F>(func)); }


public: // Internal
    auto GraphPtr() -> std::shared_ptr<GraphType>&
        { return graphPtr_; }

    auto GraphPtr() const -> const std::shared_ptr<GraphType>&
        { return graphPtr_; }

private:
    std::shared_ptr<GraphType> graphPtr_;
};

/******************************************/ REACT_END /******************************************/

#endif // REACT_GROUP_H_INCLUDED