
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

/***************************************/ REACT_IMPL_BEGIN /**************************************/

struct PrivateSignalLinkNodeInterface;

/****************************************/ REACT_IMPL_END /***************************************/

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
    SignalBase(REACT_IMPL::CtorTag, std::shared_ptr<NodeType>&& nodePtr) :
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
    auto CreateFuncNode(const std::shared_ptr<REACT_IMPL::ReactiveGraph>& graphPtr, F&& func, const SignalBase<T1>& dep1, const SignalBase<Ts>& ... deps) -> decltype(auto)
    {
        using REACT_IMPL::PrivateSignalLinkNodeInterface;
        using FuncNodeType = REACT_IMPL::SignalFuncNode<S, typename std::decay<F>::type, T1, Ts ...>;

        return std::make_shared<FuncNodeType>(
            graphPtr,
            std::forward<F>(func),
            PrivateSignalLinkNodeInterface::GetLocalNodePtr(graphPtr, dep1), PrivateSignalLinkNodeInterface::GetLocalNodePtr(graphPtr, deps) ...);
    }

    template <typename F, typename T1, typename ... Ts>
    auto CreateFuncNode(F&& func, const SignalBase<T1>& dep1, const SignalBase<Ts>& ... deps) -> decltype(auto)
    {
        using REACT_IMPL::PrivateNodeInterface;
        return CreateFuncNode(PrivateNodeInterface::GraphPtr(dep1), std::forward<F>(func), dep1, deps ...);
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

    static auto CreateVarNode(const std::shared_ptr<REACT_IMPL::ReactiveGraph>& graphPtr) -> decltype(auto)
    {
        using VarNodeType = REACT_IMPL::VarSignalNode<S>;
        return std::make_shared<VarNodeType>(graphPtr);
    }

    template <typename T>
    static auto CreateVarNode(const std::shared_ptr<REACT_IMPL::ReactiveGraph>& graphPtr, T&& value) -> decltype(auto)
    {
        using VarNodeType = REACT_IMPL::VarSignalNode<S>;
        return std::make_shared<VarNodeType>(graphPtr, std::forward<T>(value));
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

    static auto CreateSlotNode(const std::shared_ptr<REACT_IMPL::ReactiveGraph>& graphPtr, const SignalBase<S>& input) -> decltype(auto)
    {
        using REACT_IMPL::PrivateNodeInterface;
        using SlotNodeType = REACT_IMPL::SignalSlotNode<S>;

        return std::make_shared<SlotNodeType>(graphPtr, PrivateNodeInterface::NodePtr(input));
    }

private:
    void SetInput(const SignalBase<S>& newInput)
    {
        using REACT_IMPL::PrivateNodeInterface;
        using REACT_IMPL::NodeId;
        using SlotNodeType = REACT_IMPL::SignalSlotNode<S>;

        SlotNodeType* castedPtr = static_cast<SlotNodeType*>(this->NodePtr().get());

        NodeId nodeId = castedPtr->GetInputNodeId();
        auto& graphPtr = NodePtr()->GraphPtr();
        graphPtr->AddInput(nodeId, [castedPtr, &newInput] { castedPtr->SetInput(PrivateNodeInterface::NodePtr(newInput)); });
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SignalLinkBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class SignalLinkBase : public SignalBase<S>
{
public:
    using SignalBase::SignalBase;

protected:
    SignalLinkBase() = default;

    SignalLinkBase(const SignalLinkBase&) = default;
    SignalLinkBase& operator=(const SignalLinkBase&) = default;

    SignalLinkBase(SignalLinkBase&&) = default;
    SignalLinkBase& operator=(SignalLinkBase&&) = default;

    static auto CreateLinkNode(const std::shared_ptr<REACT_IMPL::ReactiveGraph>& graphPtr, const SignalBase<S>& input) -> decltype(auto)
    {
        using REACT_IMPL::PrivateNodeInterface;
        using REACT_IMPL::PrivateReactiveGroupInterface;
        using LinkNodeType = REACT_IMPL::SignalLinkNode<S>;

        auto node = std::make_shared<LinkNodeType>(graphPtr, PrivateNodeInterface::GraphPtr(input), PrivateNodeInterface::NodePtr(input));
        node->SetWeakSelfPtr(std::weak_ptr<LinkNodeType>{ node });
        return node;
    }

    friend struct REACT_IMPL::PrivateSignalLinkNodeInterface;
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

    // Construct func signal with explicit group
    template <typename F, typename ... Ts>
    explicit Signal(const ReactiveGroupBase& group, F&& func, const SignalBase<Ts>& ... deps) :
        Signal::SignalBase( REACT_IMPL::CtorTag{ }, CreateFuncNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group), std::forward<F>(func), deps ...) )
        { }

    // Construct func signal with implicit group
    template <typename F, typename ... Ts>
    explicit Signal(F&& func, const SignalBase<Ts>& ... deps) :
        Signal::SignalBase( REACT_IMPL::CtorTag{ }, CreateFuncNode(std::forward<F>(func), deps ...) )
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

    // Construct from unique
    Signal(Signal<S, unique>&& other) :
        Signal::SignalBase( std::move(other) )
        { }

    // Assign from unique
    Signal& operator=(Signal<S, unique>&& other)
        { Signal::SignalBase::operator=(std::move(other)); return *this; }

    // Construct func signal with explicit group
    template <typename F, typename ... Ts>
    explicit Signal(const ReactiveGroupBase& group, F&& func, const SignalBase<Ts>& ... deps) :
        Signal::SignalBase( REACT_IMPL::CtorTag{ }, CreateFuncNode(std::forward<F>(func), deps ...) )
        { }

    // Construct func signal with implicit group
    template <typename F, typename ... Ts>
    explicit Signal(F&& func, const SignalBase<Ts>& ... deps) :
        Signal::SignalBase( REACT_IMPL::CtorTag{ }, CreateFuncNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group), std::forward<F>(func), deps ...) )
        { }
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

    // Construct with group + default
    explicit VarSignal(const ReactiveGroupBase& group) :
        VarSignal::VarSignalBase( REACT_IMPL::CtorTag{ }, CreateVarNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group)) )
        { }

    // Construct with group + value
    template <typename T>
    VarSignal(const ReactiveGroupBase& group, T&& value) :
        VarSignal::VarSignalBase( REACT_IMPL::CtorTag{ }, CreateVarNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group), std::forward<T>(value)) )
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
    explicit VarSignal(const ReactiveGroupBase& group) :
        VarSignal::VarSignalBase( REACT_IMPL::CtorTag{ }, CreateVarNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group)) )
        { }

    // Construct with value
    template <typename T>
    VarSignal(const ReactiveGroupBase& group, T&& value) :
        VarSignal::VarSignalBase( REACT_IMPL::CtorTag{ }, CreateVarNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group), std::forward<T>(value)) )
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

    // Construct with group + default
    explicit SignalSlot(const ReactiveGroupBase& group) :
        SignalSlot::SignalSlotBase( REACT_IMPL::CtorTag{ }, CreateSlotNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group)) )
        { }

    // Construct with group + value
    SignalSlot(const ReactiveGroupBase& group, const SignalBase<S>& input) :
        SignalSlot::SignalSlotBase( REACT_IMPL::CtorTag{ }, CreateSlotNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group), input) )
        { }

    // Construct with value
    explicit SignalSlot(const SignalBase<S>& input) :
        SignalSlot::SignalSlotBase( REACT_IMPL::CtorTag{ }, CreateSlotNode(REACT_IMPL::PrivateNodeInterface::GraphPtr(input), input) )
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

    // Construct with group + default
    explicit SignalSlot(const ReactiveGroupBase& group) :
        SignalSlot::SignalSlotBase( REACT_IMPL::CtorTag{ }, CreateSlotNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group)) )
        { }

    // Construct with group + value
    SignalSlot(const ReactiveGroupBase& group, const SignalBase<S>& input) :
        SignalSlot::SignalSlotBase( REACT_IMPL::CtorTag{ }, CreateSlotNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group), input) )
        { }

    // Construct with value
    explicit SignalSlot(const SignalBase<S>& input) :
        SignalSlot::SignalSlotBase( REACT_IMPL::CtorTag{ }, CreateSlotNode(REACT_IMPL::PrivateNodeInterface::GraphPtr(input), input) )
        { }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SignalLink
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class SignalLink<S, unique> : public SignalLinkBase<S>
{
public:
    using SignalLinkBase::SignalLinkBase;

    using ValueType = S;

    SignalLink() = delete;

    SignalLink(const SignalLink&) = delete;
    SignalLink& operator=(const SignalLink&) = delete;

    SignalLink(SignalLink&&) = default;
    SignalLink& operator=(SignalLink&&) = default;

    // Construct with default
    explicit SignalLink(const ReactiveGroupBase& group) :
        SignalLink::SignalLinkBase( REACT_IMPL::CtorTag{ }, CreateLinkNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group)) )
        { }

    // Construct with group + value
    SignalLink(const ReactiveGroupBase& group, const SignalBase<S>& input) :
        SignalLink::SignalLinkBase( REACT_IMPL::CtorTag{ }, CreateLinkNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group), input) )
        { }

    // Construct with value
    explicit SignalLink(const SignalBase<S>& input) :
        SignalLink::SignalLinkBase( REACT_IMPL::CtorTag{ }, CreateLinkNode(REACT_IMPL::PrivateNodeInterface::GraphPtr(input), input) )
        { }
};

template <typename S>
class SignalLink<S, shared> : public SignalLinkBase<S>
{
public:
    using SignalLinkBase::SignalLinkBase;

    using ValueType = S;

    SignalLink() = delete;

    SignalLink(const SignalLink&) = default;
    SignalLink& operator=(const SignalLink&) = default;

    SignalLink(SignalLink&&) = default;
    SignalLink& operator=(SignalLink&&) = default;

    // Construct from unique
    SignalLink(SignalLink<S, unique>&& other) :
        SignalLink::SignalLinkBase( std::move(other) )
        { }

    // Assign from unique
    SignalLink& operator=(SignalSlot<S, unique>&& other)
        { SignalLink::SignalLinkBase::operator=(std::move(other)); return *this; }

    // Construct with default
    explicit SignalLink(const ReactiveGroupBase& group) :
        SignalLink::SignalLinkBase( REACT_IMPL::CtorTag{ }, CreateLinkNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group)) )
        { }

    // Construct with value
    SignalLink(const ReactiveGroupBase& group, const SignalBase<S>& input) :
        SignalLink::SignalLinkBase( REACT_IMPL::CtorTag{ }, CreateLinkNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group), input) )
        { }
};

/******************************************/ REACT_END /******************************************/

/***************************************/ REACT_IMPL_BEGIN /**************************************/

template <typename L, typename R>
bool Equals(const SignalBase<L>& lhs, const SignalBase<R>& rhs)
{
    return lhs.Equals(rhs);
}

struct PrivateSignalLinkNodeInterface
{
    template <typename S>
    static auto GetLocalNodePtr(const std::shared_ptr<ReactiveGraph>& targetGraph, const SignalBase<S>& sig) -> std::shared_ptr<SignalNode<S>>
    {
        const std::shared_ptr<ReactiveGraph>& sourceGraph = PrivateNodeInterface::GraphPtr(sig);

        if (sourceGraph == targetGraph)
        {
            return PrivateNodeInterface::NodePtr(sig);
        }
        else
        {
            return SignalLinkBase<S>::CreateLinkNode(targetGraph, sig);
        }
    }
};

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