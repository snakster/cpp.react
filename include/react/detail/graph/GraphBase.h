
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/detail/Defs.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>

#include "tbb/tick_count.h" // vc12 chrono clock too inaccurate

#include "react/common/Util.h"
#include "react/common/Types.h"
#include "react/detail/IReactiveEngine.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// NodeUpdateTimer
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D, bool enabled>
class NodeUpdateTimer;

// Defines guard that does nothing
template <typename D>
class NodeUpdateTimer<D,false>
{
public:
    template <typename T>
    struct ScopedUpdateTimer
    {
        ScopedUpdateTimer(const T& node) {}
    };
};

template <typename D>
class NodeUpdateTimer<D,true>
{
public:
    template <typename T>
    class ScopedUpdateTimer
    {
    public:
        ScopedUpdateTimer(T& node) :
            node_{ node }
        {
            if (node.shouldMeasure_)
                t0_ = tbb::tick_count::now();
        }

        ~ScopedUpdateTimer()
        {
            if (!node_.shouldMeasure_)
                return;

            node_.shouldMeasure_ = false;

            auto d = std::chrono::duration<double>(
                (tbb::tick_count::now() - t0_).seconds());

            D::Engine::HintUpdateDuration(
                node_, std::chrono::duration_cast<UpdateDurationT>(d).count());
        }

    private:
        T&  node_;
        tbb::tick_count t0_;
    };

private:
    bool shouldMeasure_ = true;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// NodeBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class NodeBase :
    public std::enable_shared_from_this<NodeBase<D>>,
    public D::Policy::Engine::NodeT
{
public:
    using PtrT = std::shared_ptr<NodeBase>;

    using Domain = D;
    using Policy = typename D::Policy;
    using Engine = typename D::Engine;
    using NodeT = typename Engine::NodeT;
    using TurnT = typename Engine::TurnT;

    virtual const char* GetNodeType() const override { return "NodeBase"; }
    
    virtual bool    IsInputNode() const override    { return false; }
    virtual bool    IsOutputNode() const override   { return false; }
    virtual bool    IsDynamicNode() const override  { return false; }

    virtual int     DependencyCount() const         { return 0; }

    PtrT GetSharedPtr() const
    {
        return shared_from_this();
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ReactiveNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename P,
    typename V
>
class ReactiveNode :
    public NodeBase<D>,
    public NodeUpdateTimer<D, EnableNodeUpdateTimer<typename D::Policy::Engine>::value>
{
public:
    using PtrT = std::shared_ptr<ReactiveNode>;

    ReactiveNode() = default;

    ~ReactiveNode()
    {
        if (GetObsCount() > 0)
            D::Observers().UnregisterFrom(this);
    }

    virtual const char* GetNodeType() const override { return "ReactiveNode"; }

    void    IncObsCount()       { obsCount_.fetch_add(1, std::memory_order_relaxed); }
    void    DecObsCount()       { obsCount_.fetch_sub(1, std::memory_order_relaxed); }
    uint    GetObsCount() const { return obsCount_.load(std::memory_order_relaxed); }

private:
    std::atomic<uint>   obsCount_   = 0;
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
    ReactiveOpBase(uint /*not move ctor*/, TDepsIn&& ... deps) :
        deps_{ std::forward<TDepsIn>(deps) ... }
    {}

    ReactiveOpBase(ReactiveOpBase&& other) :
        deps_{ std::move(other.deps_) }
    {}

    // Can't be copied, only moved
    ReactiveOpBase(const ReactiveOpBase& other) = delete;

    template <typename D, typename TNode>
    void Attach(TNode& node) const
    {
        apply(AttachFunctor<D,TNode>{ node }, deps_);
    }

    template <typename D, typename TNode>
    void Detach(TNode& node) const
    {
        apply(DetachFunctor<D,TNode>{ node }, deps_);
    }

    template <typename D, typename TNode, typename TFunctor>
    void AttachRec(const TFunctor& functor) const
    {
        // Same memory layout, different func
        apply(reinterpret_cast<const AttachFunctor<D,TNode>&>(functor), deps_);
    }

    template <typename D, typename TNode, typename TFunctor>
    void DetachRec(const TFunctor& functor) const
    {
        apply(reinterpret_cast<const DetachFunctor<D,TNode>&>(functor), deps_);
    }

protected:
    template <typename T>
    using NodeHolderT = std::shared_ptr<T>;

    // Attach
    template <typename D, typename TNode>
    struct AttachFunctor
    {
        AttachFunctor(TNode& node) : MyNode{ node }
        {}

        void operator()(const TDeps& ...deps) const
        {
            REACT_EXPAND_PACK(attach(deps));
        }

        template <typename T>
        void attach(const T& op) const
        {
            op.AttachRec<D,TNode>(*this);
        }

        template <typename T>
        void attach(const NodeHolderT<T>& depPtr) const
        {
            D::Engine::OnNodeAttach(MyNode, *depPtr);
        }

        TNode& MyNode;
    };

    // Detach
    template <typename D, typename TNode>
    struct DetachFunctor
    {
        DetachFunctor(TNode& node) : MyNode{ node }
        {}

        void operator()(const TDeps& ... deps) const
        {
            REACT_EXPAND_PACK(detach(deps));
        }

        template <typename T>
        void detach(const T& op) const
        {
            op.DetachRec<D,TNode>(*this);
        }

        template <typename T>
        void detach(const NodeHolderT<T>& depPtr) const
        {
            D::Engine::OnNodeDetach(MyNode, *depPtr);
        }

        TNode& MyNode;
    };

    // Dependency counting
    template <typename T>
    struct CountHelper { static const int value = T::dependency_count; };

    template <typename T>
    struct CountHelper<NodeHolderT<T>> { static const int value = 1; };

    template <int N, typename... Args>
    struct DepCounter;

    template <>
    struct DepCounter<0> { static int const value = 0; };

    template <int N, typename First, typename... Args>
    struct DepCounter<N, First, Args...>
    {
        static int const value =
            CountHelper<std::decay<First>::type>::value + DepCounter<N-1,Args...>::value;
    };

public:
    static const int dependency_count = DepCounter<sizeof...(TDeps), TDeps...>::value;

protected:
    DepHolderT   deps_;
};

/****************************************/ REACT_IMPL_END /***************************************/
