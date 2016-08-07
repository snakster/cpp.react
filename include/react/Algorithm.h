
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

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Hold - Hold the most recent event in a signal
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T, typename E>
auto Hold(T&& initialValue, const EventBase<E>& events) -> Signal<E, unique>
{
    using REACT_IMPL::HoldNode;
    using REACT_IMPL::PrivateNodeInterface;
    using REACT_IMPL::NodeCtorTag;

    return Signal<E, unique>( NodeCtorTag{ }, std::make_shared<HoldNode<E>>(
        PrivateNodeInterface::GraphPtr(events), std::forward<T>(initialValue), PrivateNodeInterface::NodePtr(events)) );
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Monitor - Emits value changes of target signal
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
auto Monitor(const SignalBase<S>& signal) -> Event<S, unique>
{
    using REACT_IMPL::MonitorNode;
    using REACT_IMPL::PrivateNodeInterface;
    using REACT_IMPL::NodeCtorTag;

    return Event<S, unique>( NodeCtorTag{ }, std::make_shared<MonitorNode<S>>(
        PrivateNodeInterface::GraphPtr(signal), PrivateNodeInterface::NodePtr(signal)) );
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Iterate - Iteratively combines signal value with values from event stream (aka Fold)
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename T, typename F, typename E>
auto Iterate(T&& init, F&& func, const EventBase<E>& events) -> Signal<S, unique>
{
    using REACT_IMPL::IterateNode;
    using REACT_IMPL::IterateByRefNode;
    using REACT_IMPL::IsCallableWith;
    using REACT_IMPL::PrivateNodeInterface;
    using REACT_IMPL::NodeCtorTag;

    using FuncType = typename std::decay<F>::type;
    using IterNodeType = typename std::conditional<
        IsCallableWith<F,S,EventRange<E>,S>::value,
        IterateNode<S, FuncType, E>,
        IterateByRefNode<S, FuncType, E>>::type;

    return Signal<S, unique>( NodeCtorTag{ }, std::make_shared<IterNodeType>(
        PrivateNodeInterface::GraphPtr(events), std::forward<T>(init), std::forward<F>(func), PrivateNodeInterface::NodePtr(events) ));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Iterate - Synced
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename T, typename F, typename E, typename ... Us>
auto Iterate(T&& init, F&& func, const EventBase<E>& events, const SignalBase<Us>& ... signals) -> Signal<S, unique>
{
    using REACT_IMPL::SyncedIterateNode;
    using REACT_IMPL::SyncedIterateByRefNode;
    using REACT_IMPL::IsCallableWith;
    using REACT_IMPL::GetCheckedGraphPtr;
    using REACT_IMPL::PrivateNodeInterface;
    using REACT_IMPL::NodeCtorTag;

    using FuncType = typename std::decay<F>::type;
    using IterNodeType = typename std::conditional<
        IsCallableWith<F, S, EventRange<E>, S, Us ...>::value,
        SyncedIterateNode<S, FuncType, E, Us ...>,
        SyncedIterateByRefNode<S, FuncType, E, Us ...>>::type;

    return Signal<S, unique>( NodeCtorTag{ }, std::make_shared<IterNodeType>(
        GetCheckedGraphPtr(events, signals ...),
        std::forward<T>(init), std::forward<F>(func), PrivateNodeInterface::NodePtr(events), PrivateNodeInterface::NodePtr(signals) ...));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Snapshot - Sets signal value to value of other signal when event is received
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename E>
auto Snapshot(const SignalBase<S>& signal, const EventBase<E>& trigger) -> Signal<S, unique>
{
    using REACT_IMPL::SnapshotNode;
    using REACT_IMPL::GetCheckedGraphPtr;
    using REACT_IMPL::PrivateNodeInterface;
    using REACT_IMPL::NodeCtorTag;

    return Events<S, unique>( NodeCtorTag{ }, std::make_shared<SnapshotNode<S, E>>(
        GetCheckedGraphPtr(signal, trigger), PrivateNodeInterface::NodePtr(signal), PrivateNodeInterface::NodePtr(trigger)) );
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Pulse - Emits value of target signal when event is received
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename E>
auto Pulse(const SignalBase<S>& signal, const EventBase<E>& trigger) -> Event<S, unique>
{
    using REACT_IMPL::PulseNode;
    using REACT_IMPL::GetCheckedGraphPtr;
    using REACT_IMPL::PrivateNodeInterface;

    return Event<S, unique>( NodeCtorTag{ }, std::make_shared<PulseNode<S, E>>(
        GetCheckedGraphPtr(signal, trigger), PrivateNodeInterface::NodePtr(signal), PrivateNodeInterface::NodePtr(trigger)) );
}

/******************************************/ REACT_END /******************************************/

#endif // REACT_ALGORITHM_H_INCLUDED