
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
#include "react/detail/state_nodes.h"

#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// State
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class State : protected REACT_IMPL::StateInternals<S>
{
public:
    // Construct with explicit group
    template <typename F, typename T1, typename ... Ts>
    static State Create(const Group& group, F&& func, const State<T1>& dep1, const State<Ts>& ... deps)
        { return CreateFuncNode(group, std::forward<F>(func), dep1, deps ...); }

    // Construct with implicit group
    template <typename F, typename T1, typename ... Ts>
    static State Create(F&& func, const State<T1>& dep1, const State<Ts>& ... deps)
        { return CreateFuncNode(dep1.GetGroup(), std::forward<F>(func), dep1, deps ...); }

    // Construct with constant value
    template <typename T>
    static State Create(const Group& group, T&& init)
        { return CreateFuncNode(group, [value = std::move(init)] { return value; }); }

    State() = default;

    State(const State&) = default;
    State& operator=(const State&) = default;

    State(State&&) = default;
    State& operator=(State&&) = default;

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
    State(std::shared_ptr<REACT_IMPL::StateNode<S>>&& nodePtr) :
        State::StateInternals( std::move(nodePtr) )
    { }

private:
    template <typename F, typename T1, typename ... Ts>
    static auto CreateFuncNode(const Group& group, F&& func, const State<T1>& dep1, const State<Ts>& ... deps) -> decltype(auto)
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
    // Construct with group + default
    static StateVar Create(const Group& group)
        { return CreateVarNode(group); }

    // Construct with group + value
    template <typename T>
    static StateVar Create(const Group& group, T&& value)
        { return CreateVarNode(group, std::forward<T>(value)); }

    StateVar() = default;

    StateVar(const StateVar&) = default;
    StateVar& operator=(const StateVar&) = default;

    StateVar(StateVar&&) = default;
    StateVar& operator=(StateVar&&) = default;

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
    StateVar(std::shared_ptr<REACT_IMPL::StateNode<S>>&& nodePtr) :
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
    // Construct with explicit group
    static StateSlot Create(const Group& group, const State<S>& input)
        { return CreateSlotNode(group, input); }

    // Construct with implicit group
    static StateSlot Create(const State<S>& input)
        { return CreateSlotNode(input.GetGroup(), input); }

    StateSlot() = default;

    StateSlot(const StateSlot&) = default;
    StateSlot& operator=(const StateSlot&) = default;

    StateSlot(StateSlot&&) = default;
    StateSlot& operator=(StateSlot&&) = default;

    void Set(const State<S>& newInput)
        { SetInput(newInput); }

    void operator<<=(const State<S>& newInput)
        { SetInput(newInput); }

protected:
    StateSlot(std::shared_ptr<REACT_IMPL::StateNode<S>>&& nodePtr) :
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
    // Construct with group
    static StateLink Create(const Group& group, const State<S>& input)
        { return GetOrCreateLinkNode(group, input); }

    StateLink() = default;

    StateLink(const StateLink&) = default;
    StateLink& operator=(const StateLink&) = default;

    StateLink(StateLink&&) = default;
    StateLink& operator=(StateLink&&) = default;

protected:
    StateLink(std::shared_ptr<REACT_IMPL::StateNode<S>>&& nodePtr) :
        StateLink::State( std::move(nodePtr) )
    { }

private:
    static auto GetOrCreateLinkNode(const Group& group, const State<S>& input) -> decltype(auto)
    {
        using REACT_IMPL::StateLinkNode;
        using REACT_IMPL::IReactNode;
        using REACT_IMPL::ReactGraph;
        
        IReactNode* k = GetInternals(input).GetNodePtr().get();

        ReactGraph::LinkCache& linkCache = GetInternals(group).GetGraphPtr()->GetLinkCache();

        std::shared_ptr<IReactNode> nodePtr = linkCache.LookupOrCreate(k, [&]
            {
                auto nodePtr = std::make_shared<StateLinkNode<S>>(group, input);
                nodePtr->SetWeakSelfPtr(std::weak_ptr<StateLinkNode<S>>{ nodePtr });
                return std::static_pointer_cast<IReactNode>(nodePtr);
            });

        return std::static_pointer_cast<StateLinkNode<S>>(nodePtr);
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ObjectContext
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class ObjectContext
{
public:
    ObjectContext() = default;

    ObjectContext(const ObjectContext&) = default;
    ObjectContext& operator=(const ObjectContext&) = default;

    ObjectContext(ObjectContext&&) = default;
    ObjectContext& operator=(ObjectContext&&) = default;

    S& GetObject()
        { return *objectPtr_; }

    const S& GetObject() const
        { return *objectPtr_; }

    template <typename U>
    const U& Get(const State<U>& member) const
        { return GetInternals(member).Value(); }

    template <typename U>
    const EventValueList<U>& Get(const Event<U>& member) const
        { return GetInternals(member).Events(); }

private:
    template <typename ... Us>
    explicit ObjectContext(S* objectPtr) :
        objectPtr_( objectPtr )
    { }

    S* objectPtr_;

    template <typename U>
    friend class impl::StateNode;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ObjectState
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class ObjectState : public State<ObjectContext<S>>
{
public:
    // Construct with group
    template <typename ... Us>
    static ObjectState Create(const Group& group, S&& obj, const Us& ... members)
    {
        using REACT_IMPL::NodeId;

        std::initializer_list<NodeId> memberIds = { GetInternals(members).GetNodeId() ... };

        return CreateObjectStateNode(group, std::move(obj), memberIds);
    }

    template <typename ... Us>
    static ObjectState Create(InPlaceTag, const Group& group, Us&& ... args)
        { return CreateObjectStateNode(in_place, group, std::forward<Us>(args) ...); }

    ObjectState() = default;

    ObjectState(const ObjectState&) = default;
    ObjectState& operator=(const ObjectState&) = default;

    ObjectState(ObjectState&&) = default;
    ObjectState& operator=(ObjectState&&) = default;

    S* operator->()
        { return &this->Value().GetObject(); }

protected:
    ObjectState(std::shared_ptr<REACT_IMPL::StateNode<ObjectContext<S>>>&& nodePtr) :
        ObjectState::State( std::move(nodePtr) )
    { }

private:
    static auto CreateObjectStateNode(const Group& group, S&& obj, const std::initializer_list<REACT_IMPL::NodeId>& memberIds) -> decltype(auto)
    {
        using REACT_IMPL::ObjectStateNode;

        return std::make_shared<ObjectStateNode<S>>(group, std::move(obj), memberIds);
    }

    template <typename ... Us>
    static auto CreateObjectStateNode(InPlaceTag, const Group& group, Us&& ... args) -> decltype(auto)
    {
        using REACT_IMPL::ObjectStateNode;
        using REACT_IMPL::SameGroupOrLink;

        return std::make_shared<ObjectStateNode<S>>(in_place, group, std::forward<Us>(args) ...);
    }
};

/******************************************/ REACT_END /******************************************/

#endif // REACT_STATE_H_INCLUDED