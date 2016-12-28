
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
    using REACT_IMPL::PrivateEventLinkNodeInterface;
    using REACT_IMPL::PrivateGroupInterface;
    using REACT_IMPL::PrivateNodeInterface;
    using REACT_IMPL::CtorTag;

    const auto& graphPtr = PrivateGroupInterface::GraphPtr(group);

    return PrivateNodeInterface::CreateNodeHelper<Signal<E>, HoldNode<E>>(
        graphPtr, std::forward<T>(initialValue), PrivateEventLinkNodeInterface::GetLocalNodePtr(graphPtr, evnt));
}

template <typename T, typename E>
auto Hold(T&& initialValue, const Event<E>& evnt) -> Signal<E>
{
    using REACT_IMPL::HoldNode;
    using REACT_IMPL::PrivateEventLinkNodeInterface;
    using REACT_IMPL::PrivateNodeInterface;
    using REACT_IMPL::CtorTag;

    const auto& graphPtr = PrivateNodeInterface::GraphPtr(evnt);

    return PrivateNodeInterface::CreateNodeHelper<Signal<E>, HoldNode<E>>(
        graphPtr, std::forward<T>(initialValue), PrivateEventLinkNodeInterface::GetLocalNodePtr(graphPtr, evnt));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Monitor - Emits value changes of target signal
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
auto Monitor(const Group& group, const Signal<S>& signal) -> Event<S>
{
    using REACT_IMPL::MonitorNode;
    using REACT_IMPL::PrivateSignalLinkNodeInterface;
    using REACT_IMPL::PrivateGroupInterface;
    using REACT_IMPL::PrivateNodeInterface;

    const auto& graphPtr = PrivateGroupInterface::GraphPtr(group);

    return PrivateNodeInterface::CreateNodeHelper<Event<S>, MonitorNode<S>>(
        graphPtr, PrivateSignalLinkNodeInterface::GetLocalNodePtr(graphPtr, signal));
}

template <typename S>
auto Monitor(const Signal<S>& signal) -> Event<S>
{
    using REACT_IMPL::MonitorNode;
    using REACT_IMPL::PrivateSignalLinkNodeInterface;
    using REACT_IMPL::PrivateNodeInterface;

    const auto& graphPtr = PrivateNodeInterface::GraphPtr(signal);

    return PrivateNodeInterface::CreateNodeHelper<Event<S>, MonitorNode<S>>(
        graphPtr, PrivateSignalLinkNodeInterface::GetLocalNodePtr(graphPtr, signal));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Iterate - Iteratively combines signal value with values from event stream (aka Fold)
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename T, typename F, typename E>
auto Iterate(const Group& group, T&& initialValue, F&& func, const Event<E>& evnt) -> Signal<S>
{
    using REACT_IMPL::IterateNode;
    using REACT_IMPL::IterateByRefNode;
    using REACT_IMPL::IsCallableWith;
    using REACT_IMPL::PrivateEventLinkNodeInterface;
    using REACT_IMPL::PrivateGroupInterface;
    using REACT_IMPL::PrivateNodeInterface;

    using FuncType = typename std::decay<F>::type;
    using IterNodeType = typename std::conditional<
        IsCallableWith<F,S,EventRange<E>,S>::value,
        IterateNode<S, FuncType, E>,
        IterateByRefNode<S, FuncType, E>>::type;

    const auto& graphPtr = PrivateGroupInterface::GraphPtr(group);

    return PrivateNodeInterface::CreateNodeHelper<Signal<S>, IterNodeType>(
        graphPtr, std::forward<T>(initialValue), std::forward<F>(func), PrivateEventLinkNodeInterface::GetLocalNodePtr(graphPtr, evnt));
}

template <typename S, typename T, typename F, typename E>
auto Iterate(T&& initialValue, F&& func, const Event<E>& evnt) -> Signal<S>
{
    using REACT_IMPL::IterateNode;
    using REACT_IMPL::IterateByRefNode;
    using REACT_IMPL::IsCallableWith;
    using REACT_IMPL::PrivateEventLinkNodeInterface;
    using REACT_IMPL::PrivateNodeInterface;
    using REACT_IMPL::CtorTag;

    using FuncType = typename std::decay<F>::type;
    using IterNodeType = typename std::conditional<
        IsCallableWith<F,S,EventRange<E>,S>::value,
        IterateNode<S, FuncType, E>,
        IterateByRefNode<S, FuncType, E>>::type;

    const auto& graphPtr = PrivateNodeInterface::GraphPtr(evnt);

    return PrivateNodeInterface::CreateNodeHelper<Signal<S>, IterNodeType>(
        graphPtr, std::forward<T>(initialValue), std::forward<F>(func), PrivateEventLinkNodeInterface::GetLocalNodePtr(graphPtr, evnt));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Iterate - Synced
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename T, typename F, typename E, typename ... Us>
auto Iterate(const Group& group, T&& initialValue, F&& func, const Event<E>& evnt, const Signal<Us>& ... signals) -> Signal<S>
{
    using REACT_IMPL::SyncedIterateNode;
    using REACT_IMPL::SyncedIterateByRefNode;
    using REACT_IMPL::IsCallableWith;
    using REACT_IMPL::PrivateSignalLinkNodeInterface;
    using REACT_IMPL::PrivateEventLinkNodeInterface;
    using REACT_IMPL::PrivateGroupInterface;
    using REACT_IMPL::PrivateNodeInterface;

    using FuncType = typename std::decay<F>::type;
    using IterNodeType = typename std::conditional<
        IsCallableWith<F, S, EventRange<E>, S, Us ...>::value,
        SyncedIterateNode<S, FuncType, E, Us ...>,
        SyncedIterateByRefNode<S, FuncType, E, Us ...>>::type;

    const auto& graphPtr = PrivateGroupInterface::GraphPtr(group);

    return PrivateNodeInterface::CreateNodeHelper<Signal<S>, IterNodeType>(
        graphPtr, std::forward<T>(initialValue), std::forward<F>(func),
        PrivateEventLinkNodeInterface::GetLocalNodePtr(graphPtr, evnt), PrivateSignalLinkNodeInterface::GetLocalNodePtr(graphPtr, signals) ...);
}

template <typename S, typename T, typename F, typename E, typename ... Us>
auto Iterate(T&& initialValue, F&& func, const Event<E>& evnt, const Signal<Us>& ... signals) -> Signal<S>
{
    using REACT_IMPL::SyncedIterateNode;
    using REACT_IMPL::SyncedIterateByRefNode;
    using REACT_IMPL::IsCallableWith;
    using REACT_IMPL::PrivateSignalLinkNodeInterface;
    using REACT_IMPL::PrivateEventLinkNodeInterface;;
    using REACT_IMPL::PrivateNodeInterface;

    using FuncType = typename std::decay<F>::type;
    using IterNodeType = typename std::conditional<
        IsCallableWith<F, S, EventRange<E>, S, Us ...>::value,
        SyncedIterateNode<S, FuncType, E, Us ...>,
        SyncedIterateByRefNode<S, FuncType, E, Us ...>>::type;

    const auto& graphPtr = PrivateNodeInterface::GraphPtr(evnt);

    return PrivateNodeInterface::CreateNodeHelper<Signal<S>, IterNodeType>(
        graphPtr, std::forward<T>(initialValue), std::forward<F>(func),
        PrivateEventLinkNodeInterface::GetLocalNodePtr(graphPtr, evnt), PrivateSignalLinkNodeInterface::GetLocalNodePtr(graphPtr, signals) ...);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Snapshot - Sets signal value to value of other signal when event is received
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename E>
auto Snapshot(const Group& group, const Signal<S>& signal, const Event<E>& evnt) -> Signal<S>
{
    using REACT_IMPL::SnapshotNode;
    using REACT_IMPL::PrivateSignalLinkNodeInterface;
    using REACT_IMPL::PrivateEventLinkNodeInterface;;
    using REACT_IMPL::PrivateGroupInterface;
    using REACT_IMPL::PrivateNodeInterface;

    const auto& graphPtr = PrivateGroupInterface::GraphPtr(group);

    return PrivateNodeInterface::CreateNodeHelper<Signal<S>, SnapshotNode<S, E>>(
        graphPtr, PrivateSignalLinkNodeInterface::GetLocalNodePtr(graphPtr, signal), PrivateEventLinkNodeInterface::GetLocalNodePtr(graphPtr, evnt));
}

template <typename S, typename E>
auto Snapshot(const Signal<S>& signal, const Event<E>& evnt) -> Signal<S>
{
    using REACT_IMPL::SnapshotNode;
    using REACT_IMPL::PrivateSignalLinkNodeInterface;
    using REACT_IMPL::PrivateEventLinkNodeInterface;;
    using REACT_IMPL::PrivateNodeInterface;

    const auto& graphPtr = PrivateNodeInterface::GraphPtr(signal);

    return PrivateNodeInterface::CreateNodeHelper<Signal<S>, SnapshotNode<S, E>>(
        graphPtr, PrivateSignalLinkNodeInterface::GetLocalNodePtr(graphPtr, signal), PrivateEventLinkNodeInterface::GetLocalNodePtr(graphPtr, evnt));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Pulse - Emits value of target signal when event is received
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename E>
auto Pulse(const Group& group, const Signal<S>& signal, const Event<E>& evnt) -> Event<S>
{
    using REACT_IMPL::PulseNode;
    using REACT_IMPL::PrivateSignalLinkNodeInterface;
    using REACT_IMPL::PrivateEventLinkNodeInterface;;
    using REACT_IMPL::PrivateGroupInterface;
    using REACT_IMPL::PrivateNodeInterface;

    const auto& graphPtr = PrivateGroupInterface::GraphPtr(group);

    return PrivateNodeInterface::CreateNodeHelper<Event<S>, PulseNode<S, E>>(
        graphPtr, PrivateSignalLinkNodeInterface::GetLocalNodePtr(graphPtr, signal), PrivateEventLinkNodeInterface::GetLocalNodePtr(graphPtr, evnt));
}

template <typename S, typename E>
auto Pulse(const Signal<S>& signal, const Event<E>& evnt) -> Event<S>
{
    using REACT_IMPL::PulseNode;
    using REACT_IMPL::PrivateSignalLinkNodeInterface;
    using REACT_IMPL::PrivateEventLinkNodeInterface;;
    using REACT_IMPL::PrivateNodeInterface;
    using REACT_IMPL::CtorTag;

    const auto& graphPtr = PrivateNodeInterface::GraphPtr(signal);

    return PrivateNodeInterface::CreateNodeHelper<Event<S>, PulseNode<S, E>>(
        graphPtr, PrivateSignalLinkNodeInterface::GetLocalNodePtr(graphPtr, signal), PrivateEventLinkNodeInterface::GetLocalNodePtr(graphPtr, evnt));
}

/******************************************/ REACT_END /******************************************/

#endif // REACT_ALGORITHM_H_INCLUDED