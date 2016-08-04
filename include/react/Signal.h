
//          Copyright Sebastian Jeckel 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_SIGNAL_H_INCLUDED
#define REACT_SIGNAL_H_INCLUDED

#pragma once

#if _MSC_VER && !__INTEL_COMPILER
    #pragma warning(disable : 4503)
#endif

#include "react/detail/Defs.h"
#include "react/API.h"
#include "react/Group.h"

#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#include "react/detail/graph/SignalNodes.h"

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SignalBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
class SignalBase
{
private:
    using NodeType = REACT_IMPL::SignalNode<T>;

public:
    SignalBase() = default;

    SignalBase(const SignalBase&) = default;
    SignalBase& operator=(const SignalBase&) = default;

    SignalBase(SignalBase&&) = default;
    SignalBase& operator=(SignalBase&&) = default;

    ~SignalBase() = default;

    // Internal node ctor
    explicit SignalBase(std::shared_ptr<NodeType>&& nodePtr) :
        nodePtr_( std::move(nodePtr) )
        { }

    const T& Value() const
        { return nodePtr_->Value(); }

protected:
    auto NodePtr() -> std::shared_ptr<NodeType>&
        { return nodePtr_; }

    auto NodePtr() const -> const std::shared_ptr<NodeType>&
        { return nodePtr_; }

    template <typename FIn, typename U1, typename ... Us>
    auto CreateFuncNode(FIn&& func, const SignalBase<U1>& dep1, const SignalBase<Us>& ... deps) -> decltype(auto)
    {
        using F = typename std::decay<FIn>::type;
        using FuncNodeType = REACT_IMPL::SignalFuncNode<T, F, U1, Us ...>;

        return std::make_shared<FuncNodeType>(
            REACT_IMPL::GetCheckedGraphPtr(dep1, deps ...),
            std::forward<FIn>(func),
            REACT_IMPL::PrivateNodeInterface::NodePtr(dep1), REACT_IMPL::PrivateNodeInterface::NodePtr(deps) ...);
    }

private:
    std::shared_ptr<NodeType> nodePtr_;

    friend struct REACT_IMPL::PrivateNodeInterface;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// VarSignalBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
class VarSignalBase : public SignalBase<T>
{
public:
    using SignalBase::SignalBase;

    VarSignalBase() = default;

    VarSignalBase(const VarSignalBase&) = default;
    VarSignalBase& operator=(const VarSignalBase&) = default;

    VarSignalBase(VarSignalBase&&) = default;
    VarSignalBase& operator=(VarSignalBase&&) = default;

    void Set(const T& newValue)
        { SetValue(newValue); }

    void Set(T&& newValue)
        { SetValue(std::move(newValue)); }

    void operator<<=(const T& newValue)
        { SetValue(newValue); }

    void operator<<=(T&& newValue)
        { SetValue(std::move(newValue)); }

    template <typename F>
    void Modify(const F& func)
        { ModifyValue(func); }

protected:
    template <typename U, typename TGroup>
    auto CreateVarNode(U&& value, const TGroup& group) -> decltype(auto)
    {
        using VarNodeType = REACT_IMPL::VarSignalNode<T>;
        return std::make_shared<VarNodeType>(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group), std::forward<U>(value));
    }

private:
    template <typename U>
    void SetValue(U&& newValue)
    {
        using REACT_IMPL::NodeId;
        using REACT_IMPL::IReactiveGraph;
        using VarNodeType = REACT_IMPL::VarSignalNode<T>;

        VarNodeType* castedPtr = static_cast<VarNodeType*>(this->NodePtr().get());

        NodeId nodeId = castedPtr->GetNodeId();
        auto& graphPtr = NodePtr()->GraphPtr();
        graphPtr->AddInput(nodeId, [castedPtr, &newValue] { castedPtr->SetValue(std::forward<U>(newValue)); });
    }

    template <typename F>
    void ModifyValue(const F& func)
    {
        using REACT_IMPL::NodeId;
        using REACT_IMPL::IReactiveGraph;
        using VarNodeType = REACT_IMPL::VarSignalNode<T>;

        VarNodeType* castedPtr = static_cast<VarNodeType*>(this->NodePtr().get());
        
        NodeId nodeId = castedPtr->GetNodeId();
        auto& graphPtr = castedPtr->GraphPtr();
        graphPtr->AddInput(nodeId, [castedPtr, &func] { castedPtr->ModifyValue(func); });
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Signal
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
class Signal<T, unique> : public SignalBase<T>
{
public:
    using SignalBase::SignalBase;

    using ValueType = T;

    Signal() = delete;

    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;

    Signal(Signal&&) = default;
    Signal& operator=(Signal&&) = default;

    template <typename F, typename ... Us>
    Signal(F&& func, const SignalBase<Us>& ... deps) :
        SignalBase( CreateFuncNode(std::forward<F>(func), deps ...) )
        { }
};

template <typename T>
class Signal<T, shared> : public SignalBase<T>
{
public:
    using SignalBase::SignalBase;

    using ValueType = T;

    Signal() = delete;

    Signal(const Signal&) = default;
    Signal& operator=(const Signal&) = default;

    Signal(Signal&&) = default;
    Signal& operator=(Signal&&) = default;

    Signal(Signal<T, unique>&& other) :
        Signal::SignalBase( std::move(other) )
        { }

    Signal& operator=(Signal<T, unique>&& other)
        { Signal::SignalBase::operator=(std::move(other)); return *this; }

    template <typename F, typename ... Us>
    Signal(F&& func, const SignalBase<Us>& ... deps) :
        Signal::SignalBase( CreateFuncNode(std::forward<F>(func), deps ...) )
        { }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Signal
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
class VarSignal<T, unique> : public VarSignalBase<T>
{
public:
    using VarSignalBase::VarSignalBase;

    using ValueType = T;

    VarSignal() = delete;

    VarSignal(const VarSignal&) = delete;
    VarSignal& operator=(const VarSignal&) = delete;

    VarSignal(VarSignal&&) = default;
    VarSignal& operator=(VarSignal&&) = default;

    template <typename U, typename TGroup>
    VarSignal(U&& value, const TGroup& group) :
        VarSignal::VarSignalBase( CreateVarNode(std::forward<U>(value), group) )
        { }
};

template <typename T>
class VarSignal<T, shared> : public VarSignalBase<T>
{
public:
    using VarSignalBase::VarSignalBase;

    using ValueType = T;

    VarSignal() = delete;

    VarSignal(const VarSignal&) = default;
    VarSignal& operator=(const VarSignal&) = default;

    VarSignal(VarSignal&&) = default;
    VarSignal& operator=(VarSignal&&) = default;

    VarSignal(VarSignal<T, unique>&& other) :
        VarSignal::VarSignalBase( std::move(other) )
        { }

    VarSignal& operator=(VarSignal<T, unique>&& other)
        { VarSignal::SignalBase::operator=(std::move(other)); return *this; }

    template <typename U, typename TGroup>
    VarSignal(U&& value, const TGroup& group) :
        VarSignal::VarSignalBase( CreateVarNode(std::forward<U>(value), group) )
        { }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Flatten
///////////////////////////////////////////////////////////////////////////////////////////////////
/*template <typename TInner, EOwnershipPolicy ownership_policy>
auto Flatten(const SignalBase<SignalBase<TInner>>& outer) -> Signal<TInner, ownership_policy>
{
    return Signal<TInner>(
        std::make_shared<REACT_IMPL::FlattenNode<Signal<TInner>, TInner>>(
            GetNodePtr(outer), GetNodePtr(outer.Value())));
}*/

/******************************************/ REACT_END /******************************************/

/***************************************/ REACT_IMPL_BEGIN /**************************************/

template <typename L, typename R>
bool Equals(const SignalBase<L>& lhs, const SignalBase<R>& rhs)
{
    return lhs.Equals(rhs);
}

/****************************************/ REACT_IMPL_END /***************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Flatten macros
///////////////////////////////////////////////////////////////////////////////////////////////////
// Note: Using static_cast rather than -> return type, because when using lambda for inline
// class initialization, decltype did not recognize the parameter r
// Note2: MSVC doesn't like typename in the lambda
#if _MSC_VER && !__INTEL_COMPILER
    #define REACT_MSVC_NO_TYPENAME
#else
    #define REACT_MSVC_NO_TYPENAME typename
#endif

#define REACTIVE_REF(obj, name)                                                             \
    Flatten(                                                                                \
        MakeSignal(                                                                         \
            obj,                                                                            \
            [] (const REACT_MSVC_NO_TYPENAME                                                \
                REACT_IMPL::Identity<decltype(obj)>::Type::ValueT& r)                       \
            {                                                                               \
                using T = decltype(r.name);                                                 \
                using S = REACT_MSVC_NO_TYPENAME REACT::DecayInput<T>::Type;                \
                return static_cast<S>(r.name);                                              \
            }))

#define REACTIVE_PTR(obj, name)                                                             \
    Flatten(                                                                                \
        MakeSignal(                                                                         \
            obj,                                                                            \
            [] (REACT_MSVC_NO_TYPENAME                                                      \
                REACT_IMPL::Identity<decltype(obj)>::Type::ValueT r)                        \
            {                                                                               \
                assert(r != nullptr);                                                       \
                using T = decltype(r->name);                                                \
                using S = REACT_MSVC_NO_TYPENAME REACT::DecayInput<T>::Type;                \
                return static_cast<S>(r->name);                                             \
            }))

#endif // REACT_SIGNAL_H_INCLUDED