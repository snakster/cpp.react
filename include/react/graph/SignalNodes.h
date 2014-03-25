
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/Defs.h"

#include <memory>
#include <functional>
#include <tuple>
#include <utility>

#include "GraphBase.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

template <typename D, typename L, typename R>
bool Equals(const L& lhs, const R& rhs);

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SignalNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S
>
class SignalNode : public ReactiveNode<D,S,S>
{
public:
    using PtrT      = std::shared_ptr<SignalNode>;
    using WeakPtrT  = std::weak_ptr<SignalNode>;

    using MergedOpT = std::pair<std::function<S(const S&,const S&)>, PtrT>;
    using MergedOpVectT = std::vector<MergedOpT>;

    explicit SignalNode(bool registered) :
        ReactiveNode(true)
    {
        if (!registered)
            registerNode();
    }

    template <typename T>
    SignalNode(T&& value, bool registered) :
        ReactiveNode{ true },
        value_{ std::forward<T>(value) }
    {
        if (!registered)
            registerNode();
    }

    ~SignalNode()
    {
        if (mergedOps_ != nullptr)
            for (const auto& mo : *mergedOps_)
                Engine::OnNodeDetach(*this, *mo.second);
    }

    virtual const char* GetNodeType() const override { return "SignalNode"; }

    virtual ETickResult Tick(void* turnPtr) override
    {
        REACT_ASSERT(false, "Don't tick SignalNode\n");
        return ETickResult::none;
    }

    const S& ValueRef() const
    {
        return value_;
    }

    template <typename F>
    void MergeOp(const PtrT& dep, F&& func)
    {
        if (mergedOps_ == nullptr)
            mergedOps_ = std::unique_ptr<MergedOpVectT>(new MergedOpVectT());

        mergedOps_->emplace_back(std::forward<F>(func),  dep);

        Engine::OnNodeAttach(*this, *dep);

        value_ = func(value_, dep->ValueRef());
    }

protected:
    S applyMergedOps(S newValue) const
    {
        for (const auto& mo : *mergedOps_)
            newValue = mo.first(newValue, mo.second->ValueRef());
        return std::move(newValue);
    }

    S value_;

    std::unique_ptr<MergedOpVectT>    mergedOps_ = nullptr;
};

template <typename D, typename S>
using SignalNodePtr = typename SignalNode<D,S>::PtrT;

template <typename D, typename S>
using SignalNodeWeakPtr = typename SignalNode<D,S>::WeakPtrT;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// VarNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S
>
class VarNode : public SignalNode<D,S>
{
public:
    template <typename T>
    VarNode(T&& value, bool registered) :
        SignalNode<D,S>(std::forward<T>(value), true),
        newValue_{ value_ }
    {
        if (!registered)
            registerNode();
    }

    virtual const char* GetNodeType() const override { return "VarNode"; }

    virtual ETickResult Tick(void* turnPtr) override
    {
        if (! impl::Equals(value_, newValue_))
        {
            using TurnT = typename D::Engine::TurnT;
            TurnT& turn = *static_cast<TurnT*>(turnPtr);

            value_ = std::move(newValue_);
            Engine::OnTurnInputChange(*this, turn);
            return ETickResult::pulsed;
        }
        else
        {
            return ETickResult::none;
        }
    }

    virtual bool IsInputNode() const override    { return true; }

    template <typename V>
    void AddInput(V&& newValue)
    {
        newValue_ = std::forward<V>(newValue);
    }

private:
    S   newValue_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ValNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S
>
class ValNode : public SignalNode<D,S>
{
public:
    template <typename T>
    ValNode(T&& value, bool registered) :
        SignalNode<D,S>(std::forward<T>(value), true)
    {
        if (!registered)
            registerNode();
    }

    virtual const char* GetNodeType() const override { return "ValNode"; }

    virtual ETickResult Tick(void* turnPtr) override
    {
        REACT_ASSERT(false, "Don't tick the ValNode\n");
        return ETickResult::none;
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// FunctionNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S,
    typename ... TArgs
>
class FunctionNode : public SignalNode<D,S>
{
public:
    template <typename F>
    FunctionNode(F&& func, const SignalNodePtr<D,TArgs>& ... args, bool registered) :
        SignalNode<D, S>(true),
        deps_{ make_tuple(args ...) },
        func_{ std::forward<F>(func) }
    {
        if (!registered)
            registerNode();

        value_ = func_(args->ValueRef() ...);

        REACT_EXPAND_PACK(Engine::OnNodeAttach(*this, *args));
    }

    ~FunctionNode()
    {
        apply
        (
            [this] (const SignalNodePtr<D,TArgs>& ... args)
            {
                REACT_EXPAND_PACK(Engine::OnNodeDetach(*this, *args));
            },
            deps_
        );
    }

    virtual const char* GetNodeType() const override { return "FunctionNode"; }

    virtual ETickResult Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *static_cast<TurnT*>(turnPtr);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        S newValue = evaluate();
        if (mergedOps_ != nullptr)
            newValue = applyMergedOps(std::move(newValue));

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (! impl::Equals(value_, newValue))
        {
            value_ = std::move(newValue);
            Engine::OnNodePulse(*this, turn);
            return ETickResult::pulsed;
        }
        else
        {
            Engine::OnNodeIdlePulse(*this, turn);
            return ETickResult::idle_pulsed;
        }
    }

    virtual int DependencyCount() const override    { return sizeof ... (TArgs); }

private:
    const std::tuple<SignalNodePtr<D,TArgs> ...>    deps_;
    const std::function<S(const TArgs& ...)>        func_;

    S evaluate() const
    {
        return apply(func_, apply(unpackValues, deps_));
    }

    static inline auto unpackValues(const SignalNodePtr<D,TArgs>& ... args)
        -> decltype(std::tie(args->ValueRef() ...))
    {
        return std::tie(args->ValueRef() ...);
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// FlattenNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename TOuter,
    typename TInner
>
class FlattenNode : public SignalNode<D,TInner>
{
public:
    FlattenNode(const SignalNodePtr<D,TOuter>& outer, const SignalNodePtr<D,TInner>& inner, bool registered) :
        SignalNode<D, TInner>(inner->ValueRef(), true),
        outer_{ outer },
        inner_{ inner }
    {
        if (!registered)
            registerNode();

        Engine::OnNodeAttach(*this, *outer_);
        Engine::OnNodeAttach(*this, *inner_);
    }

    ~FlattenNode()
    {
        Engine::OnNodeDetach(*this, *inner_);
        Engine::OnNodeDetach(*this, *outer_);
    }

    virtual const char* GetNodeType() const override { return "FlattenNode"; }

    virtual bool IsDynamicNode() const override    { return true; }

    virtual ETickResult Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *static_cast<TurnT*>(turnPtr);

        auto newInner = outer_->ValueRef().GetPtr();

        if (newInner != inner_)
        {
            // Topology has been changed
            auto oldInner = inner_;
            inner_ = newInner;

            Engine::OnDynamicNodeDetach(*this, *oldInner, turn);
            Engine::OnDynamicNodeAttach(*this, *newInner, turn);

            return ETickResult::invalidated;
        }

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        TInner newValue = inner_->ValueRef();

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (newValue != value_)
        {
            value_ = newValue;
            Engine::OnNodePulse(*this, turn);
            return ETickResult::pulsed;
        }
        else
        {
            Engine::OnNodeIdlePulse(*this, turn);
            return ETickResult::idle_pulsed;
        }
    }

    virtual int DependencyCount() const override    { return 2; }

private:
    SignalNodePtr<D,TOuter>    outer_;
    SignalNodePtr<D,TInner>    inner_;
};

/****************************************/ REACT_IMPL_END /***************************************/
