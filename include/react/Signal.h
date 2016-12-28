
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
/// Signal
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class Signal
{
private:
    using NodeType = REACT_IMPL::SignalNode<S>;

public:
    Signal(const Signal&) = default;
    Signal& operator=(const Signal&) = default;

    Signal(Signal&&) = default;
    Signal& operator=(Signal&&) = default;

    // Construct func signal with explicit group
    template <typename F, typename T1, typename ... Ts>
    explicit Signal(const Group& group, F&& func, const Signal<T1>& dep1, const Signal<Ts>& ... deps) :
        Signal::Signal( REACT_IMPL::CtorTag{ }, CreateFuncNode(REACT_IMPL::PrivateGroupInterface::GraphPtr(group), std::forward<F>(func), dep1, deps ...) )
        { }

    // Construct func signal with implicit group
    template <typename F, typename T1, typename ... Ts>
    explicit Signal(F&& func, const Signal<T1>& dep1, const Signal<Ts>& ... deps) :
        Signal::Signal( REACT_IMPL::CtorTag{ }, CreateFuncNode(REACT_IMPL::PrivateNodeInterface::GraphPtr(dep1), std::forward<F>(func), dep1, deps ...) )
        { }

    const Group& GetGroup() const
        { return nodePtr_->GetGroup(); }

public: // Internal
    // Private node ctor
    Signal(REACT_IMPL::CtorTag, std::shared_ptr<NodeType>&& nodePtr) :
        nodePtr_( std::move(nodePtr) )
        { }

    auto NodePtr() -> std::shared_ptr<NodeType>&
        { return nodePtr_; }

    auto NodePtr() const -> const std::shared_ptr<NodeType>&
        { return nodePtr_; }

protected:
    template <typename F, typename T1, typename ... Ts>
    auto CreateFuncNode(const std::shared_ptr<REACT_IMPL::ReactiveGraph>& graphPtr, F&& func, const Signal<T1>& dep1, const Signal<Ts>& ... deps) -> decltype(auto)
    {
        using REACT_IMPL::PrivateSignalLinkNodeInterface;
        using FuncNodeType = REACT_IMPL::SignalFuncNode<S, typename std::decay<F>::type, T1, Ts ...>;

        return std::make_shared<FuncNodeType>(
            graphPtr,
            std::forward<F>(func),
            PrivateSignalLinkNodeInterface::GetLocalNodePtr(graphPtr, dep1), PrivateSignalLinkNodeInterface::GetLocalNodePtr(graphPtr, deps) ...);
    }

private:
    std::shared_ptr<NodeType> nodePtr_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// VarSignal
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class VarSignal : public Signal<S>
{
public:
    VarSignal(const VarSignal&) = default;
    VarSignal& operator=(const VarSignal&) = default;

    VarSignal(VarSignal&&) = default;
    VarSignal& operator=(VarSignal&&) = default;

    // Construct with group + default
    explicit VarSignal(const Group& group) :
        VarSignal::Signal( REACT_IMPL::CtorTag{ }, CreateVarNode(group) )
        { }

    // Construct with group + value
    template <typename T>
    VarSignal(const Group& group, T&& value) :
        VarSignal::Signal( REACT_IMPL::CtorTag{ }, CreateVarNode(group, std::forward<T>(value)) )
        { }

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
    static auto CreateVarNode(const Group& group) -> decltype(auto)
    {
        using VarNodeType = REACT_IMPL::VarSignalNode<S>;
        return std::make_shared<VarNodeType>(graphPtr);
    }

    template <typename T>
    static auto CreateVarNode(const Group& group, T&& value) -> decltype(auto)
    {
        using VarNodeType = REACT_IMPL::VarSignalNode<S>;
        return std::make_shared<VarNodeType>(graphPtr, std::forward<T>(value));
    }

private:
    template <typename T>
    void SetValue(T&& newValue)
    {
        using REACT_IMPL::NodeId;
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
class SignalSlot : public Signal<S>
{
public:
    SignalSlot(const SignalSlot&) = default;
    SignalSlot& operator=(const SignalSlot&) = default;

    SignalSlot(SignalSlot&&) = default;
    SignalSlot& operator=(SignalSlot&&) = default;

    // Construct with group + default
    explicit SignalSlot(const Group& group) :
        SignalSlot::Signal( REACT_IMPL::CtorTag{ }, CreateSlotNode(REACT_IMPL::PrivateGroupInterface::GraphPtr(group)) )
        { }

    // Construct with group + value
    SignalSlot(const Group& group, const Signal<S>& input) :
        SignalSlot::Signal( REACT_IMPL::CtorTag{ }, CreateSlotNode(REACT_IMPL::PrivateGroupInterface::GraphPtr(group), input) )
        { }

    // Construct with value
    explicit SignalSlot(const Signal<S>& input) :
        SignalSlot::Signal( REACT_IMPL::CtorTag{ }, CreateSlotNode(REACT_IMPL::PrivateNodeInterface::GraphPtr(input), input) )
        { }

    void Set(const Signal<S>& newInput)
        { SetInput(newInput); }

    void operator<<=(const Signal<S>& newInput)
        { SetInput(newInput); }

protected:
    static auto CreateSlotNode(const Group& group, const Signal<S>& input) -> decltype(auto)
    {
        using REACT_IMPL::PrivateNodeInterface;
        using SlotNodeType = REACT_IMPL::SignalSlotNode<S>;

        return std::make_shared<SlotNodeType>(graphPtr, PrivateNodeInterface::NodePtr(input));
    }

private:
    void SetInput(const Signal<S>& newInput)
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
/// SignalLink
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class SignalLink : public Signal<S>
{
public:
    SignalLink(const SignalLink&) = default;
    SignalLink& operator=(const SignalLink&) = default;

    SignalLink(SignalLink&&) = default;
    SignalLink& operator=(SignalLink&&) = default;

    // Construct with explicit group
    SignalLink(const Group& group, const Signal<S>& input) :
        SignalLink::Signal( REACT_IMPL::CtorTag{ }, CreateLinkNode(group, input) )
        { }

    // Construct with implicit group
    explicit SignalLink(const Signal<S>& input) :
        SignalLink::Signal( REACT_IMPL::CtorTag{ }, CreateLinkNode(input.GetGroup(), input) )
        { }

protected:
    static auto CreateLinkNode(const Group& group, const Signal<S>& input) -> decltype(auto)
    {
        using REACT_IMPL::PrivateNodeInterface;
        using REACT_IMPL::PrivateGroupInterface;
        using LinkNodeType = REACT_IMPL::SignalLinkNode<S>;

        auto node = std::make_shared<LinkNodeType>(graphPtr, PrivateNodeInterface::GraphPtr(input), PrivateNodeInterface::NodePtr(input));
        node->SetWeakSelfPtr(std::weak_ptr<LinkNodeType>{ node });
        return node;
    }

    friend struct REACT_IMPL::PrivateSignalLinkNodeInterface;
};

/******************************************/ REACT_END /******************************************/

/***************************************/ REACT_IMPL_BEGIN /**************************************/

template <typename L, typename R>
bool Equals(const Signal<L>& lhs, const Signal<R>& rhs)
{
    return lhs.Equals(rhs);
}

struct PrivateSignalLinkNodeInterface
{
    template <typename S>
    static auto GetLocalNodePtr(const std::shared_ptr<ReactiveGraph>& targetGraph, const Signal<S>& sig) -> std::shared_ptr<SignalNode<S>>
    {
        const std::shared_ptr<ReactiveGraph>& sourceGraph = PrivateNodeInterface::GraphPtr(sig);

        if (sourceGraph == targetGraph)
        {
            return PrivateNodeInterface::NodePtr(sig);
        }
        else
        {
            return SignalLink<S>::CreateLinkNode(targetGraph, sig);
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