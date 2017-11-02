
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_ALGORITHM_H_INCLUDED
#define REACT_ALGORITHM_H_INCLUDED

#pragma once

#include "react/detail/defs.h"

#include <memory>
#include <type_traits>
#include <utility>

#include "react/api.h"
#include "react/state.h"
#include "react/event.h"

#include "react/detail/algorithm_nodes.h"

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Holds the most recent event in a state
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T, typename E>
auto Hold(const Group& group, T&& initialValue, const Event<E>& evnt) -> State<E>
{
    using REACT_IMPL::HoldNode;
    using REACT_IMPL::SameGroupOrLink;
    using REACT_IMPL::CreateWrappedNode;

    return CreateWrappedNode<State<E>, HoldNode<E>>(
        group, std::forward<T>(initialValue), SameGroupOrLink(group, evnt));
}

template <typename T, typename E>
auto Hold(T&& initialValue, const Event<E>& evnt) -> State<E>
    { return Hold(evnt.GetGroup(), std::forward<T>(initialValue), evnt); }

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Emits value changes of target state.
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
auto Monitor(const Group& group, const State<S>& state) -> Event<S>
{
    using REACT_IMPL::MonitorNode;
    using REACT_IMPL::SameGroupOrLink;
    using REACT_IMPL::CreateWrappedNode;

    return CreateWrappedNode<Event<S>, MonitorNode<S>>(
        group, SameGroupOrLink(group, state));
}

template <typename S>
auto Monitor(const State<S>& state) -> Event<S>
    { return Monitor(state.GetGroup(), state); }

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Iteratively combines state value with values from event stream (aka Fold)
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename T, typename F, typename E>
auto Iterate(const Group& group, T&& initialValue, F&& func, const Event<E>& evnt) -> State<S>
{
    using REACT_IMPL::IterateNode;
    using REACT_IMPL::SameGroupOrLink;
    using REACT_IMPL::CreateWrappedNode;

    using FuncType = typename std::decay<F>::type;

    return CreateWrappedNode<State<S>, IterateNode<S, FuncType, E>>(
        group, std::forward<T>(initialValue), std::forward<F>(func), SameGroupOrLink(group, evnt));
}

template <typename S, typename T, typename F, typename E>
auto IterateByRef(const Group& group, T&& initialValue, F&& func, const Event<E>& evnt) -> State<S>
{
    using REACT_IMPL::IterateByRefNode;
    using REACT_IMPL::SameGroupOrLink;
    using REACT_IMPL::CreateWrappedNode;

    using FuncType = typename std::decay<F>::type;

    return CreateWrappedNode<State<S>, IterateByRefNode<S, FuncType, E>>(
        group, std::forward<T>(initialValue), std::forward<F>(func), SameGroupOrLink(group, evnt));
}

template <typename S, typename T, typename F, typename E>
auto Iterate(T&& initialValue, F&& func, const Event<E>& evnt) -> State<S>
    { return Iterate<S>(evnt.GetGroup(), std::forward<T>(initialValue), std::forward<F>(func), evnt); }

template <typename S, typename T, typename F, typename E>
auto IterateByRef(T&& initialValue, F&& func, const Event<E>& evnt) -> State<S>
    { return IterateByRef<S>(evnt.GetGroup(), std::forward<T>(initialValue), std::forward<F>(func), evnt); }

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Iterate - Synced
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename T, typename F, typename E, typename ... Us>
auto Iterate(const Group& group, T&& initialValue, F&& func, const Event<E>& evnt, const State<Us>& ... states) -> State<S>
{
    using REACT_IMPL::SyncedIterateNode;
    using REACT_IMPL::SameGroupOrLink;
    using REACT_IMPL::CreateWrappedNode;

    using FuncType = typename std::decay<F>::type;

    return CreateWrappedNode<State<S>, SyncedIterateNode<S, FuncType, E, Us ...>>(
        group, std::forward<T>(initialValue), std::forward<F>(func), SameGroupOrLink(group, evnt), SameGroupOrLink(group, states) ...);
}

template <typename S, typename T, typename F, typename E, typename ... Us>
auto IterateByRef(const Group& group, T&& initialValue, F&& func, const Event<E>& evnt, const State<Us>& ... states) -> State<S>
{
    using REACT_IMPL::SyncedIterateByRefNode;
    using REACT_IMPL::SameGroupOrLink;
    using REACT_IMPL::CreateWrappedNode;

    using FuncType = typename std::decay<F>::type;

    return CreateWrappedNode<State<S>, SyncedIterateByRefNode<S, FuncType, E, Us ...>>(
        group, std::forward<T>(initialValue), std::forward<F>(func), SameGroupOrLink(group, evnt), SameGroupOrLink(group, states) ...);
}

template <typename S, typename T, typename F, typename E, typename ... Us>
auto Iterate(T&& initialValue, F&& func, const Event<E>& evnt, const State<Us>& ... states) -> State<S>
    { return Iterate<S>(evnt.GetGroup(), std::forward<T>(initialValue), std::forward<F>(func), evnt, states ...); }

template <typename S, typename T, typename F, typename E, typename ... Us>
auto IterateByRef(T&& initialValue, F&& func, const Event<E>& evnt, const State<Us>& ... states) -> State<S>
    { return IterateByRef<S>(evnt.GetGroup(), std::forward<T>(initialValue), std::forward<F>(func), evnt, states ...); }

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Snapshot - Sets state value to value of other state when event is received
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename E>
auto Snapshot(const Group& group, const State<S>& state, const Event<E>& evnt) -> State<S>
{
    using REACT_IMPL::SnapshotNode;
    using REACT_IMPL::SameGroupOrLink;
    using REACT_IMPL::CreateWrappedNode;

    return CreateWrappedNode<State<S>, SnapshotNode<S, E>>(
        group, SameGroupOrLink(group, state), SameGroupOrLink(group, evnt));
}

template <typename S, typename E>
auto Snapshot(const State<S>& state, const Event<E>& evnt) -> State<S>
    { return Snapshot(state.GetGroup(), state, evnt); }

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Pulse - Emits value of target state when event is received
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename E>
auto Pulse(const Group& group, const State<S>& state, const Event<E>& evnt) -> Event<S>
{
    using REACT_IMPL::PulseNode;
    using REACT_IMPL::SameGroupOrLink;
    using REACT_IMPL::CreateWrappedNode;

    return CreateWrappedNode<Event<S>, PulseNode<S, E>>(
        group, SameGroupOrLink(group, state), SameGroupOrLink(group, evnt));
}

template <typename S, typename E>
auto Pulse(const State<S>& state, const Event<E>& evnt) -> Event<S>
    { return Pulse(state.GetGroup(), state, evnt); }

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Flatten
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, template <typename> class TState,
    typename = std::enable_if_t<std::is_base_of_v<State<S>, TState<S>>>>
auto Flatten(const Group& group, const State<TState<S>>& state) -> State<S>
{
    using REACT_IMPL::FlattenStateNode;
    using REACT_IMPL::SameGroupOrLink;
    using REACT_IMPL::CreateWrappedNode;

    return CreateWrappedNode<State<S>, FlattenStateNode<S, TState>>(group, SameGroupOrLink(group, state));
}

template <typename S, template <typename> class TState,
    typename = std::enable_if_t<std::is_base_of_v<State<S>, TState<S>>>>
auto Flatten(const State<TState<S>>& state) -> State<S>
    { return Flatten(state.GetGroup(), state); }

///////////////////////////////////////////////////////////////////////////////////////////////////
/// FlattenList
///////////////////////////////////////////////////////////////////////////////////////////////////
template <template <typename ...> class TList, template <typename> class TState, typename V, typename ... TParams,
    typename = std::enable_if_t<std::is_base_of_v<State<V>, TState<V>>>>
auto FlattenList(const Group& group, const State<TList<TState<V>, TParams ...>>& list) -> State<TList<V>>
{
    using REACT_IMPL::FlattenStateListNode;
    using REACT_IMPL::SameGroupOrLink;
    using REACT_IMPL::CreateWrappedNode;

    return CreateWrappedNode<State<TList<V>>, FlattenStateListNode<TList, TState, V, TParams ...>>(
        group, SameGroupOrLink(group, list));
}

template <template <typename ...> class TList, template <typename> class TState, typename V, typename ... TParams,
    typename = std::enable_if_t<std::is_base_of_v<State<V>, TState<V>>>>
auto FlattenList(const State<TList<TState<V>, TParams ...>>& list) -> State<TList<V>>
    { return FlattenList(list.GetGroup(), list); }

///////////////////////////////////////////////////////////////////////////////////////////////////
/// FlattenMap
///////////////////////////////////////////////////////////////////////////////////////////////////
template <template <typename ...> class TMap, template <typename> class TState, typename K, typename V, typename ... TParams,
    typename = std::enable_if_t<std::is_base_of_v<State<V>, TState<V>>>>
auto FlattenMap(const Group& group, const State<TMap<K, TState<V>, TParams ...>>& map) -> State<TMap<K, V>>
{
    using REACT_IMPL::FlattenStateMapNode;
    using REACT_IMPL::SameGroupOrLink;
    using REACT_IMPL::CreateWrappedNode;

    return CreateWrappedNode<State<TMap<K, V>>, FlattenStateMapNode<TMap, TState, K, V, TParams ...>>(
        group, SameGroupOrLink(group, map));
}

template <template <typename ...> class TMap, template <typename> class TState, typename K, typename V, typename ... TParams,
    typename = std::enable_if_t<std::is_base_of_v<State<V>, TState<V>>>>
auto FlattenMap(const State<TMap<K, TState<V>, TParams ...>>& map) -> State<TMap<K, V>>
    { return FlattenMap(map.GetGroup(), map); }

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Flattened
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename C>
class Flattened : public C
{
public:
    using C::C;

    Flattened(const C& base) :
        C( base )
    { }

    Flattened(const C& base, REACT_IMPL::FlattenedInitTag) :
        C( base ),
        initMode_( true )
    { }

    Flattened(const C& base, REACT_IMPL::FlattenedInitTag, std::vector<REACT_IMPL::NodeId>&& emptyMemberIds) :
        C( base ),
        initMode_( true ),
        memberIds_( std::move(emptyMemberIds) ) // This will be empty, but has pre-allocated storage. It's a tweak.
    { }

    template <typename T>
    Ref<T> Flatten(State<T>& signal)
    {
        if (initMode_)
            memberIds_.push_back(GetInternals(signal).GetNodeId());
        
        return GetInternals(signal).Value();
    }

private:
    bool initMode_ = false;
    std::vector<REACT_IMPL::NodeId> memberIds_;

    template <typename T, typename TFlat>
    friend class impl::FlattenObjectNode;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// FlattenObject
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T, typename TFlat = typename T::Flat>
auto FlattenObject(const Group& group, const State<T>& obj) -> State<TFlat>
{
    using REACT_IMPL::FlattenObjectNode;
    using REACT_IMPL::SameGroupOrLink;
    using REACT_IMPL::CreateWrappedNode;

    return CreateWrappedNode<State<TFlat>, FlattenObjectNode<T, TFlat>>(group, obj);
}

template <typename T, typename TFlat = typename T::Flat>
auto FlattenObject(const State<T>& obj) -> State<TFlat>
    { return FlattenObject(obj.GetGroup(), obj); }

template <typename T, typename TFlat = typename T::Flat>
auto FlattenObject(const Group& group, const State<Ref<T>>& obj) -> State<TFlat>
{
    using REACT_IMPL::FlattenObjectNode;
    using REACT_IMPL::SameGroupOrLink;
    using REACT_IMPL::CreateWrappedNode;

    return CreateWrappedNode<State<TFlat>, FlattenObjectNode<Ref<T>, TFlat>>(group, obj);
}

template <typename T, typename TFlat = typename T::Flat>
auto FlattenObject(const State<Ref<T>>& obj) -> State<TFlat>
    { return FlattenObject(obj.GetGroup(), obj); }

/******************************************/ REACT_END /******************************************/

#endif // REACT_ALGORITHM_H_INCLUDED