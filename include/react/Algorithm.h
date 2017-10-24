
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

#include "react/API.h"
#include "react/detail/algorithm_nodes.h"

#include "react/state.h"
#include "react/event.h"

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Hold the most recent event in a state
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
/// Iterate - Iteratively combines state value with values from event stream (aka Fold)
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename T, typename F, typename E>
auto Iterate(const Group& group, T&& initialValue, F&& func, const Event<E>& evnt) -> State<S>
{
    using REACT_IMPL::IterateNode;
    using REACT_IMPL::IsCallableWith;
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
    using REACT_IMPL::IsCallableWith;
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
    using REACT_IMPL::IsCallableWith;
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
    using REACT_IMPL::IsCallableWith;
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

/******************************************/ REACT_END /******************************************/

#endif // REACT_ALGORITHM_H_INCLUDED