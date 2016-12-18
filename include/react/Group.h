
//          Copyright Sebastian Jeckel 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DOMAIN_H_INCLUDED
#define REACT_DOMAIN_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <memory>
#include <utility>

#include "react/API.h"

#include "react/detail/IReactiveGraph.h"
#include "react/detail/graph/PropagationST.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

struct PrivateReactiveGroupInterface;
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
/// ReactiveGroupBase
///////////////////////////////////////////////////////////////////////////////////////////////////
class ReactiveGroup
{
    using GraphType = REACT_IMPL::ReactiveGraph;

public:
    ReactiveGroup() :
        graphPtr_( std::make_shared<GraphType>() )
        {  }

    ReactiveGroup(const ReactiveGroup&) = default;
    ReactiveGroup& operator=(const ReactiveGroup&) = default;

    ReactiveGroup(ReactiveGroup&& other) = default;
    ReactiveGroup& operator=(ReactiveGroup&& other) = default;

    template <typename F>
    void DoTransaction(F&& func)
        { graphPtr_->DoTransaction(std::forward<F>(func)); }

    template <typename F>
    void EnqueueTransaction(F&& func)
        { EnqueueTransaction(TransactionFlags::none, std::forward<F>(func)); }

    template <typename F>
    void EnqueueTransaction(TransactionFlags flags, F&& func)
        { graphPtr_->EnqueueTransaction(flags, std::forward<F>(func)); }

protected:
    auto GraphPtr() -> std::shared_ptr<GraphType>&
        { return graphPtr_; }

    auto GraphPtr() const -> const std::shared_ptr<GraphType>&
        { return graphPtr_; }

private:
    std::shared_ptr<GraphType> graphPtr_;

    friend struct REACT_IMPL::PrivateReactiveGroupInterface;
};

/******************************************/ REACT_END /******************************************/

/***************************************/ REACT_IMPL_BEGIN /**************************************/

struct PrivateNodeInterface
{
    template <typename TBase>
    static auto NodePtr(const TBase& base) -> const std::shared_ptr<typename TBase::NodeType>&
        { return base.NodePtr(); }

    template <typename TBase>
    static auto NodePtr(TBase& base) -> std::shared_ptr<typename TBase::NodeType>&
        { return base.NodePtr(); }

    template <typename TBase>
    static auto GraphPtr(const TBase& base) -> const std::shared_ptr<ReactiveGraph>&
        { return base.NodePtr()->GraphPtr(); }

    template <typename TBase>
    static auto GraphPtr(TBase& base) -> std::shared_ptr<ReactiveGraph>&
        { return base.NodePtr()->GraphPtr(); }
};

struct PrivateReactiveGroupInterface
{
    static auto GraphPtr(const ReactiveGroup& group) -> const std::shared_ptr<ReactiveGraph>&
        { return group.GraphPtr(); }

    static auto GraphPtr(ReactiveGroup& group) -> std::shared_ptr<ReactiveGraph>&
        { return group.GraphPtr(); }
};

template <typename TBase1, typename ... TBases>
static auto GetCheckedGraphPtr(const TBase1& dep1, const TBases& ... deps) -> const std::shared_ptr<ReactiveGraph>&
{
    const std::shared_ptr<ReactiveGraph>& graphPtr1 = PrivateNodeInterface::GraphPtr(dep1);

    std::initializer_list<ReactiveGraph*> rawGraphPtrs = { PrivateNodeInterface::GraphPtr(deps).get() ... };

    bool isSameGraphForAllDeps = std::all_of(rawGraphPtrs.begin(), rawGraphPtrs.end(), [&] (ReactiveGraph* p) { return p == graphPtr1.get(); });

    REACT_ASSERT(isSameGraphForAllDeps, "All dependencies must belong to the same group.");

    return graphPtr1;
}

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DOMAIN_H_INCLUDED