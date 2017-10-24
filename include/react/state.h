
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_STATE_H_INCLUDED
#define REACT_STATE_H_INCLUDED

#pragma once

#include "react/detail/defs.h"
#include "react/api.h"
#include "react/group.h"
#include "react/common/ptrcache.h"

#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#include "react/detail/state_nodes.h"


/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// State
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class State : protected REACT_IMPL::StateInternals<S>
{
public:
    State(const State&) = default;
    State& operator=(const State&) = default;

    State(State&&) = default;
    State& operator=(State&&) = default;

    // Construct with explicit group
    template <typename F, typename T1, typename ... Ts>
    explicit State(const Group& group, F&& func, const State<T1>& dep1, const State<Ts>& ... deps) :
        State::StateInternals( CreateFuncNode(group, std::forward<F>(func), dep1, deps ...) )
    { }

    // Construct with implicit group
    template <typename F, typename T1, typename ... Ts>
    explicit State(F&& func, const State<T1>& dep1, const State<Ts>& ... deps) :
        State::StateInternals( CreateFuncNode(dep1.GetGroup(), std::forward<F>(func), dep1, deps ...) )
    { }

    auto GetGroup() const -> const Group&
        { return this->GetNodePtr()->GetGroup(); }

    auto GetGroup() -> Group&
        { return this->GetNodePtr()->GetGroup(); }

    friend bool operator==(const State<S>& a, const State<S>& b)
        { return a.GetNodePtr() == b.GetNodePtr(); }

    friend bool operator!=(const State<S>& a, const State<S>& b)
        { return !(a == b); }

    friend auto GetInternals(State<S>& s) -> REACT_IMPL::StateInternals<S>&
        { return s; }

    friend auto GetInternals(const State<S>& s) -> const REACT_IMPL::StateInternals<S>&
        { return s; }

protected:
    explicit State(std::shared_ptr<REACT_IMPL::StateNode<S>>&& nodePtr) :
        State::StateInternals( std::move(nodePtr) )
    { }

private:
    template <typename F, typename T1, typename ... Ts>
    auto CreateFuncNode(const Group& group, F&& func, const State<T1>& dep1, const State<Ts>& ... deps) -> decltype(auto)
    {
        using REACT_IMPL::StateFuncNode;
        using REACT_IMPL::SameGroupOrLink;

        return std::make_shared<StateFuncNode<S, typename std::decay<F>::type, T1, Ts ...>>(
            group, std::forward<F>(func), SameGroupOrLink(group, dep1), SameGroupOrLink(group, deps) ...);
    }

    template <typename RET, typename NODE, typename ... ARGS>
    friend RET impl::CreateWrappedNode(ARGS&& ... args);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// StateVar
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class StateVar : public State<S>
{
public:
    StateVar(const StateVar&) = default;
    StateVar& operator=(const StateVar&) = default;

    StateVar(StateVar&&) = default;
    StateVar& operator=(StateVar&&) = default;

    // Construct with group + default
    explicit StateVar(const Group& group) :
        StateVar::State( CreateVarNode(group) )
    { }

    // Construct with group + value
    template <typename T>
    StateVar(const Group& group, T&& value) :
        StateVar::State( CreateVarNode(group, std::forward<T>(value)) )
    { }

    void Set(const S& newValue)
        { SetValue(newValue); }

    void Set(S&& newValue)
        { SetValue(std::move(newValue)); }

    template <typename F>
    void Modify(const F& func)
        { ModifyValue(func); }

    friend bool operator==(const StateVar<S>& a, StateVar<S>& b)
        { return a.GetNodePtr() == b.GetNodePtr(); }

    friend bool operator!=(const StateVar<S>& a, StateVar<S>& b)
        { return !(a == b); }

protected:
    explicit StateVar(std::shared_ptr<REACT_IMPL::StateNode<S>>&& nodePtr) :
        StateVar::State( std::move(nodePtr) )
    { }

private:
    static auto CreateVarNode(const Group& group) -> decltype(auto)
    {
        using REACT_IMPL::StateVarNode;
        return std::make_shared<StateVarNode<S>>(group);
    }

    template <typename T>
    static auto CreateVarNode(const Group& group, T&& value) -> decltype(auto)
    {
        using REACT_IMPL::StateVarNode;
        return std::make_shared<StateVarNode<S>>(group, std::forward<T>(value));
    }

    template <typename T>
    void SetValue(T&& newValue)
    {
        using REACT_IMPL::NodeId;
        using VarNodeType = REACT_IMPL::StateVarNode<S>;

        VarNodeType* castedPtr = static_cast<VarNodeType*>(this->GetNodePtr().get());

        NodeId nodeId = castedPtr->GetNodeId();
        auto& graphPtr = GetInternals(this->GetGroup()).GetGraphPtr();

        graphPtr->PushInput(nodeId, [castedPtr, &newValue] { castedPtr->SetValue(std::forward<T>(newValue)); });
    }

    template <typename F>
    void ModifyValue(const F& func)
    {
        using REACT_IMPL::NodeId;
        using VarNodeType = REACT_IMPL::StateVarNode<S>;

        VarNodeType* castedPtr = static_cast<VarNodeType*>(this->GetNodePtr().get());
        
        NodeId nodeId = castedPtr->GetNodeId();
        auto& graphPtr = GetInternals(this->GetGroup()).GetGraphPtr();

        graphPtr->PushInput(nodeId, [castedPtr, &func] { castedPtr->ModifyValue(func); });
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// StateSlotBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class StateSlot : public State<S>
{
public:
    StateSlot(const StateSlot&) = default;
    StateSlot& operator=(const StateSlot&) = default;

    StateSlot(StateSlot&&) = default;
    StateSlot& operator=(StateSlot&&) = default;

    // Construct with explicit group
    StateSlot(const Group& group, const State<S>& input) :
        StateSlot::State( CreateSlotNode(group, input) )
    { }

    // Construct with implicit group
    explicit StateSlot(const State<S>& input) :
        StateSlot::State( CreateSlotNode(input.GetGroup(), input) )
    { }

    void Set(const State<S>& newInput)
        { SetInput(newInput); }

    void operator<<=(const State<S>& newInput)
        { SetInput(newInput); }

protected:
    explicit StateSlot(std::shared_ptr<REACT_IMPL::StateNode<S>>&& nodePtr) :
        StateSlot::State( std::move(nodePtr) )
    { }

private:
    static auto CreateSlotNode(const Group& group, const State<S>& input) -> decltype(auto)
    {
        using REACT_IMPL::StateSlotNode;
        using REACT_IMPL::SameGroupOrLink;

        return std::make_shared<StateSlotNode<S>>(group, SameGroupOrLink(group, input));
    }

    void SetInput(const State<S>& newInput)
    {
        using REACT_IMPL::NodeId;
        using REACT_IMPL::StateSlotNode;
        using REACT_IMPL::SameGroupOrLink;

        auto* castedPtr = static_cast<StateSlotNode<S>*>(this->GetNodePtr().get());

        NodeId nodeId = castedPtr->GetInputNodeId();
        auto& graphPtr = GetInternals(this->GetGroup()).GetGraphPtr();

        graphPtr->PushInput(nodeId, [this, castedPtr, &newInput] { castedPtr->SetInput(SameGroupOrLink(GetGroup(), newInput)); });
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// StateLink
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class StateLink : public State<S>
{
public:
    StateLink(const StateLink&) = default;
    StateLink& operator=(const StateLink&) = default;

    StateLink(StateLink&&) = default;
    StateLink& operator=(StateLink&&) = default;

    // Construct with group
    StateLink(const Group& group, const State<S>& input) :
        StateLink::State( GetOrCreateLinkNode(group, input) )
    { }

protected:
    static auto GetOrCreateLinkNode(const Group& group, const State<S>& input) -> decltype(auto)
    {
        using REACT_IMPL::StateLinkNode;
        using REACT_IMPL::IReactNode;
        using REACT_IMPL::ReactGraph;
        
        IReactNode* k = GetInternals(input).GetNodePtr().get();

        ReactGraph::LinkCache& linkCache = GetInternals(group).GetGraphPtr()->GetLinkCache();

        std::shared_ptr<IReactNode> nodePtr = linkCache.LookupOrCreate(
            k,
            [&]
            {
                auto nodePtr = std::make_shared<StateLinkNode<S>>(group, input);
                nodePtr->SetWeakSelfPtr(std::weak_ptr<StateLinkNode<S>>{ nodePtr });
                return std::static_pointer_cast<IReactNode>(nodePtr);
            });

        return std::static_pointer_cast<StateLinkNode<S>>(nodePtr);
    }
};

/******************************************/ REACT_END /******************************************/

/***************************************/ REACT_IMPL_BEGIN /**************************************/

template <typename S>
static State<S> SameGroupOrLink(const Group& targetGroup, const State<S>& dep)
{
    if (dep.GetGroup() == targetGroup)
        return dep;
    else
        return StateLink<S>( targetGroup, dep );
}

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_STATE_H_INCLUDED