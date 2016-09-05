
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
template <typename S>
class SignalBase
{
private:
    using NodeType = REACT_IMPL::SignalNode<S>;

public:
    // Private node ctor
    SignalBase(REACT_IMPL::NodeCtorTag, std::shared_ptr<NodeType>&& nodePtr) :
        nodePtr_( std::move(nodePtr) )
        { }

    const S& Value() const
        { return nodePtr_->Value(); }

protected:
    SignalBase() = default;

    SignalBase(const SignalBase&) = default;
    SignalBase& operator=(const SignalBase&) = default;

    SignalBase(SignalBase&&) = default;
    SignalBase& operator=(SignalBase&&) = default;

    auto NodePtr() -> std::shared_ptr<NodeType>&
        { return nodePtr_; }

    auto NodePtr() const -> const std::shared_ptr<NodeType>&
        { return nodePtr_; }

    template <typename F, typename T1, typename ... Ts>
    auto CreateFuncNode(F&& func, const SignalBase<T1>& dep1, const SignalBase<Ts>& ... deps) -> decltype(auto)
    {
        using REACT_IMPL::GetCheckedGraphPtr;
        using REACT_IMPL::PrivateNodeInterface;
        using FuncNodeType = REACT_IMPL::SignalFuncNode<S, typename std::decay<F>::type, T1, Ts ...>;

        return std::make_shared<FuncNodeType>(
            GetCheckedGraphPtr(dep1, deps ...),
            std::forward<F>(func),
            PrivateNodeInterface::NodePtr(dep1), PrivateNodeInterface::NodePtr(deps) ...);
    }

private:
    std::shared_ptr<NodeType> nodePtr_;

    friend struct REACT_IMPL::PrivateNodeInterface;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// VarSignalBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class VarSignalBase : public SignalBase<S>
{
public:
    using SignalBase::SignalBase;

    void Set(const S& newValue)
        { SetValue(newValue); }

    void Set(S&& newValue)
        { SetValue(std::move(newValue)); }

    void operator<<=(const S& newValue)
        { SetValue(newValue); }

    void operator<<=(S&& newValue)
        { SetValue(std::move(newValue)); }

    template <typename F>
    void Modify(const F& func)
        { ModifyValue(func); }

protected:
    VarSignalBase() = default;

    VarSignalBase(const VarSignalBase&) = default;
    VarSignalBase& operator=(const VarSignalBase&) = default;

    VarSignalBase(VarSignalBase&&) = default;
    VarSignalBase& operator=(VarSignalBase&&) = default;

    auto CreateVarNode(const ReactiveGroupBase& group) -> decltype(auto)
    {
        using REACT_IMPL::PrivateReactiveGroupInterface;
        using VarNodeType = REACT_IMPL::VarSignalNode<S>;

        return std::make_shared<VarNodeType>(PrivateReactiveGroupInterface::GraphPtr(group));
    }

    template <typename T>
    auto CreateVarNode(T&& value, const ReactiveGroupBase& group) -> decltype(auto)
    {
        using REACT_IMPL::PrivateReactiveGroupInterface;
        using VarNodeType = REACT_IMPL::VarSignalNode<S>;

        return std::make_shared<VarNodeType>(PrivateReactiveGroupInterface::GraphPtr(group), std::forward<T>(value));
    }

private:
    template <typename T>
    void SetValue(T&& newValue)
    {
        using REACT_IMPL::NodeId;
        using REACT_IMPL::ReactiveGraph;
        using VarNodeType = REACT_IMPL::VarSignalNode<S>;

        VarNodeType* castedPtr = static_cast<VarNodeType*>(this->NodePtr().get());

        NodeId nodeId = castedPtr->GetNodeId();
        auto& graphPtr = NodePtr()->GraphPtr();
        graphPtr->AddInput(nodeId, [castedPtr, &newValue] { castedPtr->SetValue(std::forward<T>(newValue)); });
    }

    template <typename F>
    void ModifyValue(const F& func)
    {
        using REACT_IMPL::NodeId;
        using REACT_IMPL::ReactiveGraph;
        using VarNodeType = REACT_IMPL::VarSignalNode<S>;

        VarNodeType* castedPtr = static_cast<VarNodeType*>(this->NodePtr().get());
        
        NodeId nodeId = castedPtr->GetNodeId();
        auto& graphPtr = castedPtr->GraphPtr();
        graphPtr->AddInput(nodeId, [castedPtr, &func] { castedPtr->ModifyValue(func); });
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SignalSlotBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class SignalSlotBase : public SignalBase<S>
{
public:
    using SignalBase::SignalBase;

    void Set(const SignalBase<S>& newInput)
        { SetInput(newInput); }

    void operator<<=(const SignalBase<S>& newInput)
        { SetInput(newInput); }

protected:
    SignalSlotBase() = default;

    SignalSlotBase(const SignalSlotBase&) = default;
    SignalSlotBase& operator=(const SignalSlotBase&) = default;

    SignalSlotBase(SignalSlotBase&&) = default;
    SignalSlotBase& operator=(SignalSlotBase&&) = default;

    auto CreateSlotNode(const SignalBase<S>& input, const ReactiveGroupBase& group) -> decltype(auto)
    {
        using REACT_IMPL::PrivateNodeInterface;
        using REACT_IMPL::PrivateReactiveGroupInterface;
        using SlotNodeType = REACT_IMPL::SignalSlotNode<S>;

        return std::make_shared<SlotNodeType>(PrivateReactiveGroupInterface::GraphPtr(group), PrivateNodeInterface::NodePtr(input));
    }

private:
    void SetInput(const SignalBase<S>& newInput)
    {
        using REACT_IMPL::PrivateNodeInterface;
        using REACT_IMPL::NodeId;
        using REACT_IMPL::ReactiveGraph;
        using SlotNodeType = REACT_IMPL::SignalSlotNode<S>;

        SlotNodeType* castedPtr = static_cast<SlotNodeType*>(this->NodePtr().get());

        NodeId nodeId = castedPtr->GetInputNodeId();
        auto& graphPtr = NodePtr()->GraphPtr();
        graphPtr->AddInput(nodeId, [castedPtr, &newInput] { castedPtr->SetInput(PrivateNodeInterface::NodePtr(newInput)); });
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Signal
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class Signal<S, unique> : public SignalBase<S>
{
public:
    using SignalBase::SignalBase;

    using ValueType = S;

    Signal() = delete;

    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;

    Signal(Signal&&) = default;
    Signal& operator=(Signal&&) = default;

    // Construct func signal
    template <typename F, typename ... Ts>
    explicit Signal(F&& func, const SignalBase<Ts>& ... deps) :
        Signal::SignalBase( REACT_IMPL::NodeCtorTag{ }, CreateFuncNode(std::forward<F>(func), deps ...) )
        { }
};

template <typename S>
class Signal<S, shared> : public SignalBase<S>
{
public:
    using SignalBase::SignalBase;

    using ValueType = S;

    Signal() = delete;

    Signal(const Signal&) = default;
    Signal& operator=(const Signal&) = default;

    Signal(Signal&&) = default;
    Signal& operator=(Signal&&) = default;

    // Construct func signal
    template <typename F, typename ... Ts>
    explicit Signal(F&& func, const SignalBase<Ts>& ... deps) :
        Signal::SignalBase( REACT_IMPL::NodeCtorTag{ }, CreateFuncNode(std::forward<F>(func), deps ...) )
        { }

    // Construct from unique
    Signal(Signal<S, unique>&& other) :
        Signal::SignalBase( std::move(other) )
        { }

    // Assign from unique
    Signal& operator=(Signal<S, unique>&& other)
        { Signal::SignalBase::operator=(std::move(other)); return *this; }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// VarSignal
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class VarSignal<S, unique> : public VarSignalBase<S>
{
public:
    using VarSignalBase::VarSignalBase;

    using ValueType = S;

    VarSignal() = delete;

    VarSignal(const VarSignal&) = delete;
    VarSignal& operator=(const VarSignal&) = delete;

    VarSignal(VarSignal&&) = default;
    VarSignal& operator=(VarSignal&&) = default;

    // Construct with default
    template <typename TGroup>
    explicit VarSignal(const TGroup& group) :
        VarSignal::VarSignalBase( REACT_IMPL::NodeCtorTag{ }, CreateVarNode( group) )
        { }

    // Construct with value
    template <typename T, typename TGroup>
    VarSignal(T&& value, const TGroup& group) :
        VarSignal::VarSignalBase( REACT_IMPL::NodeCtorTag{ }, CreateVarNode(std::forward<T>(value), group) )
        { }
};

template <typename S>
class VarSignal<S, shared> : public VarSignalBase<S>
{
public:
    using VarSignalBase::VarSignalBase;

    using ValueType = S;

    VarSignal() = delete;

    VarSignal(const VarSignal&) = default;
    VarSignal& operator=(const VarSignal&) = default;

    VarSignal(VarSignal&&) = default;
    VarSignal& operator=(VarSignal&&) = default;

    // Construct from unique
    VarSignal(VarSignal<S, unique>&& other) :
        VarSignal::VarSignalBase( std::move(other) )
        { }

    // Assign from unique
    VarSignal& operator=(VarSignal<S, unique>&& other)
        { VarSignal::VarSignalBase::operator=(std::move(other)); return *this; }

    // Construct with default
    template <typename TGroup>
    explicit VarSignal(const TGroup& group) :
        VarSignal::VarSignalBase( REACT_IMPL::NodeCtorTag{ }, CreateVarNode( group) )
        { }

    // Construct with value
    template <typename T, typename TGroup>
    VarSignal(T&& value, const TGroup& group) :
        VarSignal::VarSignalBase( REACT_IMPL::NodeCtorTag{ }, CreateVarNode(std::forward<T>(value), group) )
        { }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SignalSlot
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class SignalSlot<S, unique> : public SignalSlotBase<S>
{
public:
    using SignalSlotBase::SignalSlotBase;

    using ValueType = S;

    SignalSlot() = delete;

    SignalSlot(const SignalSlot&) = delete;
    SignalSlot& operator=(const SignalSlot&) = delete;

    SignalSlot(SignalSlot&&) = default;
    SignalSlot& operator=(SignalSlot&&) = default;

    // Construct with default
    explicit SignalSlot(const ReactiveGroupBase& group) :
        SignalSlot::SignalSlotBase( REACT_IMPL::NodeCtorTag{ }, CreateSlotNode( group) )
        { }

    // Construct with value
    SignalSlot(const SignalBase<S>& input, const ReactiveGroupBase& group) :
        SignalSlot::SignalSlotBase( REACT_IMPL::NodeCtorTag{ }, CreateSlotNode(input, group) )
        { }
};

template <typename S>
class SignalSlot<S, shared> : public SignalSlotBase<S>
{
public:
    using SignalSlotBase::SignalSlotBase;

    using ValueType = S;

    SignalSlot() = delete;

    SignalSlot(const SignalSlot&) = default;
    SignalSlot& operator=(const SignalSlot&) = default;

    SignalSlot(SignalSlot&&) = default;
    SignalSlot& operator=(SignalSlot&&) = default;

    // Construct from unique
    SignalSlot(SignalSlot<S, unique>&& other) :
        SignalSlot::SignalSlotBase( std::move(other) )
        { }

    // Assign from unique
    SignalSlot& operator=(SignalSlot<S, unique>&& other)
        { SignalSlot::SignalSlotBase::operator=(std::move(other)); return *this; }

    // Construct with default
    explicit SignalSlot(const ReactiveGroupBase& group) :
        SignalSlot::SignalSlotBase( REACT_IMPL::NodeCtorTag{ }, CreateSlotNode( group) )
        { }

    // Construct with value
    SignalSlot(const SignalBase<S>& input, const ReactiveGroupBase& group) :
        SignalSlot::SignalSlotBase( REACT_IMPL::NodeCtorTag{ }, CreateSlotNode(input, group) )
        { }
};

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