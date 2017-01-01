
//          Copyright Sebastian Jeckel 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_ALGORITHM_H_INCLUDED
#define REACT_ALGORITHM_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <memory>
#include <type_traits>
#include <utility>

#include "react/API.h"
#include "react/detail/graph/AlgorithmNodes.h"

#include "Event.h"
#include "Signal.h"

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Hold - Hold the most recent event in a signal
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T, typename E>
auto Hold(const Group& group, T&& initialValue, const Event<E>& evnt) -> Signal<E>
{
    using REACT_IMPL::HoldNode;
    return Signal<E>::CreateWithNode<HoldNode<E>>(group, std::forward<T>(initialValue), SameGroupOrLink(group, evnt));
}

template <typename T, typename E>
auto Hold(T&& initialValue, const Event<E>& evnt) -> Signal<E>
    { return Hold(evnt.GetGroup(), std::forward<T>(initialValue), evnt); }

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Monitor - Emits value changes of target signal
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
auto Monitor(const Group& group, const Signal<S>& signal) -> Event<S>
{
    using REACT_IMPL::MonitorNode;
    return Event<S>::CreateWithNode<MonitorNode<S>>(group, SameGroupOrLink(group, signal));
}

template <typename S>
auto Monitor(const Signal<S>& signal) -> Event<S>
    { return Monitor(signal.GetGroup(), signal); }

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Iterate - Iteratively combines signal value with values from event stream (aka Fold)
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename T, typename F, typename E>
auto Iterate(const Group& group, T&& initialValue, F&& func, const Event<E>& evnt) -> Signal<S>
{
    using REACT_IMPL::IterateNode;
    using REACT_IMPL::IterateByRefNode;
    using REACT_IMPL::IsCallableWith;

    using FuncType = typename std::decay<F>::type;
    using IterNodeType = typename std::conditional<
        IsCallableWith<F,S,EventRange<E>,S>::value,
        IterateNode<S, FuncType, E>,
        IterateByRefNode<S, FuncType, E>>::type;

    return Signal<S>::CreateWithNode<IterNodeType>(group, std::forward<T>(initialValue), std::forward<F>(func), SameGroupOrLink(group, evnt));
}

template <typename S, typename T, typename F, typename E>
auto Iterate(T&& initialValue, F&& func, const Event<E>& evnt) -> Signal<S>
    { return Iterate<S>(evnt.GetGroup(), std::forward<T>(initialValue), std::forward<F>(func), evnt); }

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Iterate - Synced
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename T, typename F, typename E, typename ... Us>
auto Iterate(const Group& group, T&& initialValue, F&& func, const Event<E>& evnt, const Signal<Us>& ... signals) -> Signal<S>
{
    using REACT_IMPL::SyncedIterateNode;
    using REACT_IMPL::SyncedIterateByRefNode;
    using REACT_IMPL::IsCallableWith;

    using FuncType = typename std::decay<F>::type;
    using IterNodeType = typename std::conditional<
        IsCallableWith<F, S, EventRange<E>, S, Us ...>::value,
        SyncedIterateNode<S, FuncType, E, Us ...>,
        SyncedIterateByRefNode<S, FuncType, E, Us ...>>::type;

    return Signal<S>::CreateWithNode<IterNodeType>(
        group, std::forward<T>(initialValue), std::forward<F>(func), SameGroupOrLink(group, evnt), SameGroupOrLink(group, signals) ...);
}

template <typename S, typename T, typename F, typename E, typename ... Us>
auto Iterate(T&& initialValue, F&& func, const Event<E>& evnt, const Signal<Us>& ... signals) -> Signal<S>
    { return Iterate<S>(evnt.GetGroup(), std::forward<T>(initialValue), std::forward<F>(func), evnt, signals ...); }

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Snapshot - Sets signal value to value of other signal when event is received
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename E>
auto Snapshot(const Group& group, const Signal<S>& signal, const Event<E>& evnt) -> Signal<S>
{
    using REACT_IMPL::SnapshotNode;
    return Signal<S>::CreateWithNode<SnapshotNode<S, E>>(group, SameGroupOrLink(group, signal), SameGroupOrLink(group, evnt));
}

template <typename S, typename E>
auto Snapshot(const Signal<S>& signal, const Event<E>& evnt) -> Signal<S>
    { return Snapshot(signal.GetGroup(), signal, evnt); }

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Pulse - Emits value of target signal when event is received
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename E>
auto Pulse(const Group& group, const Signal<S>& signal, const Event<E>& evnt) -> Event<S>
{
    using REACT_IMPL::PulseNode;
    return Event<S>::CreateWithNode<PulseNode<S, E>>(group, SameGroupOrLink(group, signal), SameGroupOrLink(group, evnt));
}

template <typename S, typename E>
auto Pulse(const Signal<S>& signal, const Event<E>& evnt) -> Event<S>
    { return Pulse(signal.GetGroup(), signal, evnt); }

/******************************************/ REACT_END /******************************************/

#endif // REACT_ALGORITHM_H_INCLUDED