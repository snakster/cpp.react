
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_SIGNAL_H_INCLUDED
#define REACT_SIGNAL_H_INCLUDED

#pragma once

#include "react/detail/defs.h"
#include "react/api.h"
#include "react/group.h"
#include "react/common/ptrcache.h"

#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#include "react/detail/signal_nodes.h"


/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Signal
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class Signal : protected REACT_IMPL::SignalInternals<S>
{
public:
    Signal(const Signal&) = default;
    Signal& operator=(const Signal&) = default;

    Signal(Signal&&) = default;
    Signal& operator=(Signal&&) = default;

    // Construct with explicit group
    template <typename F, typename T1, typename ... Ts>
    explicit Signal(const Group& group, F&& func, const Signal<T1>& dep1, const Signal<Ts>& ... deps) :
        Signal::SignalInternals( CreateFuncNode(group, std::forward<F>(func), dep1, deps ...) )
    { }

    // Construct with implicit group
    template <typename F, typename T1, typename ... Ts>
    explicit Signal(F&& func, const Signal<T1>& dep1, const Signal<Ts>& ... deps) :
        Signal::SignalInternals( CreateFuncNode(dep1.GetGroup(), std::forward<F>(func), dep1, deps ...) )
    { }

    auto GetGroup() const -> const Group&
        { return this->GetNodePtr()->GetGroup(); }

    auto GetGroup() -> Group&
        { return this->GetNodePtr()->GetGroup(); }

    friend bool operator==(const Signal<S>& a, const Signal<S>& b)
        { return a.GetNodePtr() == b.GetNodePtr(); }

    friend bool operator!=(const Signal<S>& a, const Signal<S>& b)
        { return !(a == b); }

    friend auto GetInternals(Signal<S>& s) -> REACT_IMPL::SignalInternals<S>&
        { return s; }

    friend auto GetInternals(const Signal<S>& s) -> const REACT_IMPL::SignalInternals<S>&
        { return s; }

protected:
    explicit Signal(std::shared_ptr<REACT_IMPL::SignalNode<S>>&& nodePtr) :
        Signal::SignalInternals( std::move(nodePtr) )
    { }

private:
    template <typename F, typename T1, typename ... Ts>
    auto CreateFuncNode(const Group& group, F&& func, const Signal<T1>& dep1, const Signal<Ts>& ... deps) -> decltype(auto)
    {
        using REACT_IMPL::SignalFuncNode;
        using REACT_IMPL::SameGroupOrLink;

        return std::make_shared<SignalFuncNode<S, typename std::decay<F>::type, T1, Ts ...>>(
            group, std::forward<F>(func), SameGroupOrLink(group, dep1), SameGroupOrLink(group, deps) ...);
    }

    template <typename RET, typename NODE, typename ... ARGS>
    friend RET impl::CreateWrappedNode(ARGS&& ... args);
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
        VarSignal::Signal( CreateVarNode(group) )
    { }

    // Construct with group + value
    template <typename T>
    VarSignal(const Group& group, T&& value) :
        VarSignal::Signal( CreateVarNode(group, std::forward<T>(value)) )
    { }

    void Set(const S& newValue)
        { SetValue(newValue); }

    void Set(S&& newValue)
        { SetValue(std::move(newValue)); }

    template <typename F>
    void Modify(const F& func)
        { ModifyValue(func); }

    friend bool operator==(const VarSignal<S>& a, VarSignal<S>& b)
        { return a.GetNodePtr() == b.GetNodePtr(); }

    friend bool operator!=(const VarSignal<S>& a, VarSignal<S>& b)
        { return !(a == b); }

protected:
    explicit VarSignal(std::shared_ptr<REACT_IMPL::SignalNode<S>>&& nodePtr) :
        VarSignal::Signal( std::move(nodePtr) )
    { }

private:
    static auto CreateVarNode(const Group& group) -> decltype(auto)
    {
        using REACT_IMPL::VarSignalNode;
        return std::make_shared<VarSignalNode<S>>(group);
    }

    template <typename T>
    static auto CreateVarNode(const Group& group, T&& value) -> decltype(auto)
    {
        using REACT_IMPL::VarSignalNode;
        return std::make_shared<VarSignalNode<S>>(group, std::forward<T>(value));
    }

    template <typename T>
    void SetValue(T&& newValue)
    {
        using REACT_IMPL::NodeId;
        using VarNodeType = REACT_IMPL::VarSignalNode<S>;

        VarNodeType* castedPtr = static_cast<VarNodeType*>(this->GetNodePtr().get());

        NodeId nodeId = castedPtr->GetNodeId();
        auto& graphPtr = GetInternals(this->GetGroup()).GetGraphPtr();

        graphPtr->AddInput(nodeId, [castedPtr, &newValue] { castedPtr->SetValue(std::forward<T>(newValue)); });
    }

    template <typename F>
    void ModifyValue(const F& func)
    {
        using REACT_IMPL::NodeId;
        using VarNodeType = REACT_IMPL::VarSignalNode<S>;

        VarNodeType* castedPtr = static_cast<VarNodeType*>(this->GetNodePtr().get());
        
        NodeId nodeId = castedPtr->GetNodeId();
        auto& graphPtr = GetInternals(this->GetGroup()).GetGraphPtr();

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

    // Construct with explicit group
    SignalSlot(const Group& group, const Signal<S>& input) :
        SignalSlot::Signal( CreateSlotNode(group, input) )
    { }

    // Construct with implicit group
    explicit SignalSlot(const Signal<S>& input) :
        SignalSlot::Signal( CreateSlotNode(input.GetGroup(), input) )
    { }

    void Set(const Signal<S>& newInput)
        { SetInput(newInput); }

    void operator<<=(const Signal<S>& newInput)
        { SetInput(newInput); }

protected:
    explicit SignalSlot(std::shared_ptr<REACT_IMPL::SignalNode<S>>&& nodePtr) :
        SignalSlot::Signal( std::move(nodePtr) )
    { }

private:
    static auto CreateSlotNode(const Group& group, const Signal<S>& input) -> decltype(auto)
    {
        using REACT_IMPL::SignalSlotNode;
        using REACT_IMPL::SameGroupOrLink;

        return std::make_shared<SignalSlotNode<S>>(group, SameGroupOrLink(group, input));
    }

    void SetInput(const Signal<S>& newInput)
    {
        using REACT_IMPL::NodeId;
        using REACT_IMPL::SignalSlotNode;
        using REACT_IMPL::SameGroupOrLink;

        auto* castedPtr = static_cast<SignalSlotNode<S>*>(this->GetNodePtr().get());

        NodeId nodeId = castedPtr->GetInputNodeId();
        auto& graphPtr = GetInternals(this->GetGroup()).GetGraphPtr();

        graphPtr->AddInput(nodeId, [this, castedPtr, &newInput] { castedPtr->SetInput(SameGroupOrLink(GetGroup(), newInput)); });
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

    // Construct with group
    SignalLink(const Group& group, const Signal<S>& input) :
        SignalLink::Signal( GetOrCreateLinkNode(group, input) )
    { }

protected:
    static auto GetOrCreateLinkNode(const Group& group, const Signal<S>& input) -> decltype(auto)
    {
        using REACT_IMPL::SignalLinkNode;
        using REACT_IMPL::IReactNode;
        using REACT_IMPL::ReactGraph;
        
        IReactNode* k = GetInternals(input).GetNodePtr().get();

        ReactGraph::LinkCache& linkCache = GetInternals(group).GetGraphPtr()->GetLinkCache();

        std::shared_ptr<IReactNode> nodePtr = linkCache.LookupOrCreate(
            k,
            [&]
            {
                auto nodePtr = std::make_shared<SignalLinkNode<S>>(group, input);
                nodePtr->SetWeakSelfPtr(std::weak_ptr<SignalLinkNode<S>>{ nodePtr });
                return std::static_pointer_cast<IReactNode>(nodePtr);
            });

        return std::static_pointer_cast<SignalLinkNode<S>>(nodePtr);
    }
};

/******************************************/ REACT_END /******************************************/

/***************************************/ REACT_IMPL_BEGIN /**************************************/

template <typename S>
static Signal<S> SameGroupOrLink(const Group& targetGroup, const Signal<S>& dep)
{
    if (dep.GetGroup() == targetGroup)
        return dep;
    else
        return SignalLink<S>( targetGroup, dep );
}

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_SIGNAL_H_INCLUDED