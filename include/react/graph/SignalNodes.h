
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

    using MergedOpT     = std::pair<std::function<S(const S&,const S&)>, PtrT>;
    using MergedOpVectT = std::vector<MergedOpT>;

    SignalNode() :
        ReactiveNode()
    {
    }

    template <typename T>
    SignalNode(T&& value) :
        ReactiveNode(),
        value_{ std::forward<T>(value) }
    {
    }

    virtual const char* GetNodeType() const override { return "SignalNode"; }

    virtual void Tick(void* turnPtr) override
    {
        REACT_ASSERT(false, "Don't tick SignalNode\n");
        return;
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
        return;
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
    typename ... TArgs
>
class FunctionOp
{
private:

public:
    template <typename FIn, typename ... TArgsIn>
    FunctionOp(FIn&& func, TArgsIn&& ... args) :
        deps_{ std::forward<TArgsIn>(args) ... },
        func_{ std::forward<FIn>(func) }
    {}

    FunctionOp(FunctionOp&& other) :
        deps_{ std::move(other.deps_) },
        func_{ std::move(other.func_) }
    {
    }

    // Can't be copied, only moved
    FunctionOp() = delete;
    FunctionOp(const FunctionOp& other) = delete;

    S Evaluate() const
    {
        return apply(EvalFunctor{ func_ }, deps_);
    }

    template <typename D, typename TNode>
    void Attach(TNode& node) const
    {
        return apply(AttachFunctor<D,TNode>{ node }, deps_);
    }

    template <typename D, typename TNode>
    void Detach(TNode& node) const
    {
        return apply(DetachFunctor<D,TNode>{ node }, deps_);
    }

private:
    template <typename T>
    struct Helper
    {
        static auto eval(const T& arg) -> decltype(arg.Evaluate())
        {
            return arg.Evaluate();
        }

        template <typename D, typename TNode>
        static void attach(TNode& node, const T& arg)
        {
            return arg.Attach<D>(node);
        }

        template <typename D, typename TNode>
        static void detach(TNode& node, const T& arg)
        {
            return arg.Detach<D>(node);
        }

        static const int count_value = T::dependency_count;
    };

    template <typename T>
    struct Helper<std::shared_ptr<T>>
    {
        static auto eval(const std::shared_ptr<T>& depPtr)
            -> decltype(depPtr->ValueRef())
        {
            return depPtr->ValueRef();
        }

        template <typename D, typename TNode>
        static void attach(TNode& node, const std::shared_ptr<T>& depPtr)
        {
            D::Engine::OnNodeAttach(node, *depPtr);
        }

        template <typename D, typename TNode>
        static void detach(TNode& node, const std::shared_ptr<T>& depPtr)
        {
            D::Engine::OnNodeDetach(node, *depPtr);
        }

        static const int count_value = 1;
    };

    struct EvalFunctor
    {
        EvalFunctor(const F& f) : MyFunc{ f }
        {}

        S operator()(const TArgs& ... args) const
        {
            return MyFunc(Helper<std::decay<decltype(args)>::type>::eval(args) ...);
        }

        const F& MyFunc;
    };

    template <typename D, typename TNode>
    struct AttachFunctor
    {
        AttachFunctor(TNode& node) : MyNode{ node }
        {}

        void operator()(const TArgs& ... args) const
        {
            REACT_EXPAND_PACK(Helper<std::decay<decltype(args)>::type>::attach<D>(MyNode, args));
        }

        TNode& MyNode;
    };

    template <typename D, typename TNode>
    struct DetachFunctor
    {
        DetachFunctor(TNode& node) : MyNode{ node }
        {}

        void operator()(const TArgs& ... args) const
        {
            REACT_EXPAND_PACK(Helper<std::decay<decltype(args)>::type>::detach<D>(MyNode, args));
        }

        TNode& MyNode;
    };

private:
    std::tuple<TArgs ...>   deps_;
    F                       func_;

// Dependency counting
private:
    template <int N, typename... Args>
    struct DepCounter;

    template <int N, typename First, typename... Args>
    struct DepCounter<N, First, Args...>
    {
        static int const Value =
            Helper<std::decay<First>::type>::count_value + DepCounter<N-1,Args...>::Value;
    };

    template <>
    struct DepCounter<0>
    {
        static int const Value = 0;
    };

public:
    static const int dependency_count = DepCounter<sizeof...(TArgs), TArgs...>::Value;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// OpSignalNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S,
    typename TOp
>
class OpSignalNode : public SignalNode<D,S>
{
public:
    template <typename ... TArgs>
    OpSignalNode(TArgs&& ... args) :
        SignalNode<D, S>(),
        op_{ std::forward<TArgs>(args) ... }
    {
        value_ = op_.Evaluate();

        Engine::OnNodeCreate(*this);
        op_.Attach<D>(*this);
    }

    ~OpSignalNode()
    {
        if (!wasOpStolen_)
            op_.Detach<D>(*this);
        Engine::OnNodeDestroy(*this);
    }

    virtual const char* GetNodeType() const override { return "OpSignalNode"; }

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *static_cast<TurnT*>(turnPtr);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        S newValue = op_.Evaluate();

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (! impl::Equals(value_, newValue))
        {
            value_ = std::move(newValue);
            Engine::OnNodePulse(*this, turn);
            return;
        }
        else
        {
            Engine::OnNodeIdlePulse(*this, turn);
            return;
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

        if (newValue != value_)
        {
            value_ = newValue;
            Engine::OnNodePulse(*this, turn);
            return;
        }
        else
        {
            Engine::OnNodeIdlePulse(*this, turn);
            return;
        }
    }

    virtual int DependencyCount() const override    { return 2; }

private:
    SignalNodePtr<D,TOuter>    outer_;
    SignalNodePtr<D,TInner>    inner_;
};

/****************************************/ REACT_IMPL_END /***************************************/
