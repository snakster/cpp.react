
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_GRAPH_GRAPHBASE_H_INCLUDED
#define REACT_DETAIL_GRAPH_GRAPHBASE_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <memory>
#include <utility>

#include "react/common/Util.h"
#include "react/common/Timing.h"
#include "react/common/Types.h"
#include "react/detail/IReactiveEngine.h"
#include "react/detail/ObserverBase.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// UpdateTimingPolicy
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    long long threshold
>
class UpdateTimingPolicy :
    private ConditionalTimer<threshold, D::uses_node_update_timer>
{
    using ScopedTimer = typename UpdateTimingPolicy::ScopedTimer;

public:
    class ScopedUpdateTimer : private ScopedTimer
    {
    public:
        ScopedUpdateTimer(UpdateTimingPolicy& parent) :
            ScopedTimer( parent ) {}
    };

    bool IsUpdateThresholdExceeded() const { return this->IsThresholdExceeded(); }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// DepCounter
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct CountHelper { static const int value = T::dependency_count; };

template <typename T>
struct CountHelper<std::shared_ptr<T>> { static const int value = 1; };

template <int N, typename... Args>
struct DepCounter;

template <>
struct DepCounter<0> { static int const value = 0; };

template <int N, typename First, typename... Args>
struct DepCounter<N, First, Args...>
{
    static int const value =
        CountHelper<typename std::decay<First>::type>::value + DepCounter<N-1,Args...>::value;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// NodeBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class NodeBase :
    public std::enable_shared_from_this<NodeBase<D>>,
    public D::Policy::Engine::NodeT,
    public Observable<D>
{
public:
    using DomainT = D;
    using Policy = typename D::Policy;
    using Engine = typename D::Engine;
    using NodeT = typename Engine::NodeT;
    using TurnT = typename Engine::TurnT;
    
    virtual bool    IsInputNode() const override    { return false; }
    virtual bool    IsOutputNode() const override   { return false; }
    virtual bool    IsDynamicNode() const override  { return false; }
    virtual bool    IsHeavyweight() const override  { return false; }

    std::shared_ptr<NodeBase> GetSharedPtr() const
    {
        return this->shared_from_this();
    }
};

template <typename D>
using NodeBasePtrT = std::shared_ptr<NodeBase<D>>;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ReactiveNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename P,
    typename V
>
class ReactiveNode : public NodeBase<D>
{
public:
    ReactiveNode() = default;

    // Reactive nodes can't be copied
    ReactiveNode(const ReactiveNode&) = delete;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Attach/detach helper functors
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename TNode, typename ... TDeps>
struct AttachFunctor
{
    AttachFunctor(TNode& node) : MyNode( node ) {}

    void operator()(const TDeps& ...deps) const
    {
        REACT_EXPAND_PACK(attach(deps));
    }

    template <typename T>
    void attach(const T& op) const
    {
        op.template AttachRec<D,TNode>(*this);
    }

    template <typename T>
    void attach(const std::shared_ptr<T>& depPtr) const
    {
        D::Engine::OnNodeAttach(MyNode, *depPtr);
    }

    TNode& MyNode;
};

template <typename D, typename TNode, typename ... TDeps>
struct DetachFunctor
{
    DetachFunctor(TNode& node) : MyNode( node ) {}

    void operator()(const TDeps& ... deps) const
    {
        REACT_EXPAND_PACK(detach(deps));
    }

    template <typename T>
    void detach(const T& op) const
    {
        op.template DetachRec<D,TNode>(*this);
    }

    template <typename T>
    void detach(const std::shared_ptr<T>& depPtr) const
    {
        D::Engine::OnNodeDetach(MyNode, *depPtr);
    }

    TNode& MyNode;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ReactiveOpBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename ... TDeps>
class ReactiveOpBase
{
public:
    using DepHolderT = std::tuple<TDeps...>;

    template <typename ... TDepsIn>
    ReactiveOpBase(DontMove, TDepsIn&& ... deps) :
        deps_( std::forward<TDepsIn>(deps) ... )
    {}

    ReactiveOpBase(ReactiveOpBase&& other) :
        deps_( std::move(other.deps_) )
    {}

    // Can't be copied, only moved
    ReactiveOpBase(const ReactiveOpBase& other) = delete;

    template <typename D, typename TNode>
    void Attach(TNode& node) const
    {
        apply(AttachFunctor<D,TNode,TDeps...>{ node }, deps_);
    }

    template <typename D, typename TNode>
    void Detach(TNode& node) const
    {
        apply(DetachFunctor<D,TNode,TDeps...>{ node }, deps_);
    }

    template <typename D, typename TNode, typename TFunctor>
    void AttachRec(const TFunctor& functor) const
    {
        // Same memory layout, different func
        apply(reinterpret_cast<const AttachFunctor<D,TNode,TDeps...>&>(functor), deps_);
    }

    template <typename D, typename TNode, typename TFunctor>
    void DetachRec(const TFunctor& functor) const
    {
        apply(reinterpret_cast<const DetachFunctor<D,TNode,TDeps...>&>(functor), deps_);
    }

public:
    static const int dependency_count = DepCounter<sizeof...(TDeps), TDeps...>::value;

protected:
    DepHolderT   deps_;
};


/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_GRAPH_GRAPHBASE_H_INCLUDED