
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/detail/Defs.h"

#include <memory>
#include <functional>
#include <tuple>
#include <utility>


#include <deque>

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

    SignalNode() :
        ReactiveNode()
    {}

    template <typename T>
    SignalNode(T&& value) :
        ReactiveNode(),
        value_{ std::forward<T>(value) }
    {}

    virtual const char* GetNodeType() const override { return "SignalNode"; }

    virtual void Tick(void* turnPtr) override
    {
        REACT_ASSERT(false, "Don't tick SignalNode\n");
    }

    const S& ValueRef() const
    {
        return value_;
    }

protected:
    S value_;
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
class VarNode :
    public SignalNode<D,S>,
    public IInputNode
{
public:
    template <typename T>
    VarNode(T&& value) :
        SignalNode<D,S>(std::forward<T>(value)),
        newValue_{ value_ }
    {
        Engine::OnNodeCreate(*this);
    }

    ~VarNode()
    {
        Engine::OnNodeDestroy(*this);
    }

    virtual const char* GetNodeType() const override { return "VarNode"; }

    virtual void Tick(void* turnPtr) override
    {
        REACT_ASSERT(false, "Don't tick the VarNode\n");
    }

    virtual bool IsInputNode() const override    { return true; }

    template <typename V>
    void AddInput(V&& newValue)
    {
        newValue_ = std::forward<V>(newValue);
    }

    virtual bool ApplyInput(void* turnPtr) override
    {
        if (! impl::Equals(value_, newValue_))
        {
            using TurnT = typename D::Engine::TurnT;
            TurnT& turn = *static_cast<TurnT*>(turnPtr);

            value_ = std::move(newValue_);
            Engine::OnTurnInputChange(*this, turn);
            return true;
        }
        else
        {
            return false;
        }
    }

private:
    S   newValue_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// FunctionOp
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename S,
    typename F,
    typename ... TDeps
>
class FunctionOp : public ReactiveOpBase<TDeps...>
{
public:
    template <typename FIn, typename ... TDepsIn>
    FunctionOp(FIn&& func, TDepsIn&& ... deps) :
        ReactiveOpBase(0u, std::forward<TDepsIn>(deps) ...),
        func_{ std::forward<FIn>(func) }
    {}

    FunctionOp(FunctionOp&& other) :
        ReactiveOpBase(std::move(other)),
        func_{ std::move(other.func_) }
    {}

    S Evaluate() const
    {
        return apply(EvalFunctor{ func_ }, deps_);
    }

private:
    // Eval
    struct EvalFunctor
    {
        EvalFunctor(const F& f) : MyFunc{ f }
        {}

        S operator()(const TDeps& ... args) const
        {
            return MyFunc(eval(args) ...);
        }

        template <typename T>
        static auto eval(const T& op) -> decltype(op.Evaluate())
        {
            return op.Evaluate();
        }

        template <typename T>
        static auto eval(const NodeHolderT<T>& depPtr) -> decltype(depPtr->ValueRef())
        {
            return depPtr->ValueRef();
        }

        const F& MyFunc;
    };

private:
    F   func_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SignalOpNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S,
    typename TOp
>
class SignalOpNode :
    public SignalNode<D,S>,
    public NodeUpdateTimer<D>
{
public:
    template <typename ... TArgs>
    SignalOpNode(TArgs&& ... args) :
        SignalNode<D, S>(),
        op_{ std::forward<TArgs>(args) ... }
    {
        value_ = op_.Evaluate();

        Engine::OnNodeCreate(*this);
        op_.Attach<D>(*this);
    }

    ~SignalOpNode()
    {
        if (!wasOpStolen_)
            op_.Detach<D>(*this);
        Engine::OnNodeDestroy(*this);
    }

    virtual const char* GetNodeType() const override { return "SignalOpNode"; }

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *static_cast<TurnT*>(turnPtr);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));
        
        bool changed = false;

        {// timer
            ScopedUpdateTimer<SignalOpNode> timer{ *this };

            S newValue = op_.Evaluate();

            if (! REACT_IMPL::Equals(value_, newValue))
            {
                value_ = std::move(newValue);
                changed = true;
            }
        }// ~timer

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (changed)
        {
            Engine::OnNodePulse(*this, turn);
        }
        else
        {
            Engine::OnNodeIdlePulse(*this, turn);
        }
    }

    virtual int DependencyCount() const override
    {
        return TOp::dependency_count;
    }

    TOp StealOp()
    {
        REACT_ASSERT(wasOpStolen_ == false, "Op was already stolen.");
        wasOpStolen_ = true;
        op_.Detach<D>(*this);
        return std::move(op_);
    }

private:
    TOp     op_;
    bool    wasOpStolen_ = false;
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
    FlattenNode(const SignalNodePtr<D,TOuter>& outer, const SignalNodePtr<D,TInner>& inner) :
        SignalNode<D, TInner>(inner->ValueRef()),
        outer_{ outer },
        inner_{ inner }
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *outer_);
        Engine::OnNodeAttach(*this, *inner_);
    }

    ~FlattenNode()
    {
        Engine::OnNodeDetach(*this, *inner_);
        Engine::OnNodeDetach(*this, *outer_);
        Engine::OnNodeDestroy(*this);
    }

    virtual const char* GetNodeType() const override { return "FlattenNode"; }

    virtual bool IsDynamicNode() const override    { return true; }

    virtual void Tick(void* turnPtr) override
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

            return;
        }

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        TInner newValue = inner_->ValueRef();

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (! REACT_IMPL::Equals(value_, newValue))
        {
            value_ = newValue;
            Engine::OnNodePulse(*this, turn);
        }
        else
        {
            Engine::OnNodeIdlePulse(*this, turn);
        }
    }

    virtual int DependencyCount() const override    { return 2; }

private:
    SignalNodePtr<D,TOuter>    outer_;
    SignalNodePtr<D,TInner>    inner_;
};

/****************************************/ REACT_IMPL_END /***************************************/
