
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/detail/Defs.h"

#include <memory>
#include <type_traits>
#include <utility>

#include "react/detail/graph/AlgorithmNodes.h"

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward declarations
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename S>
class Signal;

template <typename D, typename S>
class VarSignal;

template <typename D, typename E>
class Events;

template <typename D, typename E>
class EventSource;

enum class EventToken;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Fold
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename V,
    typename E,
    typename FIn,
    typename S = std::decay<V>::type
>
auto Fold(V&& init, const Events<D,E>& events, FIn&& func)
    -> Signal<D,S>
{
    using REACT_IMPL::FoldNode;

    using F = std::decay<FIn>::type;

    return Signal<D,S>(
        std::make_shared<FoldNode<D,S,E,F>>(
            std::forward<V>(init), events.NodePtr(), std::forward<FIn>(func)));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// FoldByRef - Pass current value as reference
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename V,
    typename E,
    typename FIn,
    typename S = std::decay<V>::type
>
auto FoldByRef(V&& init, const Events<D,E>& events, FIn&& func)
    -> Signal<D,S>
{
    using REACT_IMPL::FoldByRefNode;

    using F = std::decay<FIn>::type;

    return Signal<D,S>(
        std::make_shared<FoldByRefNode<D,S,E,F>>(
            std::forward<V>(init), events.NodePtr(), std::forward<FIn>(func)));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Iterate
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename V,
    typename E,
    typename FIn,
    typename S = std::decay<V>::type
>
auto Iterate(V&& init, const Events<D,E>& events, FIn&& func)
    -> Signal<D,S>
{
    using REACT_IMPL::IterateNode;

    using F = std::decay<FIn>::type;

    return Signal<D,S>(
        std::make_shared<IterateNode<D,S,E,F>>(
            std::forward<V>(init), events.NodePtr(), std::forward<FIn>(func)));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IterateByRef
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename V,
    typename E,
    typename FIn,
    typename S = std::decay<V>::type
>
auto IterateByRef(V&& init, const Events<D,E>& events, FIn&& func)
    -> Signal<D,S>
{
    using REACT_IMPL::IterateByRefNode;

    using F = std::decay<FIn>::type;

    return Signal<D,S>(
        std::make_shared<IterateByRefNode<D,S,E,F>>(
            std::forward<V>(init), events.NodePtr(), std::forward<FIn>(func)));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Hold
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename V,
    typename T = std::decay<V>::type
>
auto Hold(V&& init, const Events<D,T>& events)
    -> Signal<D,T>
{
    using REACT_IMPL::HoldNode;

    return Signal<D,T>(
        std::make_shared<HoldNode<D,T>>(
            std::forward<V>(init), events.NodePtr()));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Snapshot
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S,
    typename E
>
auto Snapshot(const Events<D,E>& trigger, const Signal<D,S>& target)
    -> Signal<D,S>
{
    using REACT_IMPL::SnapshotNode;

    return Signal<D,S>(
        std::make_shared<SnapshotNode<D,S,E>>(
            target.NodePtr(), trigger.NodePtr()));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Monitor
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S
>
auto Monitor(const Signal<D,S>& target)
    -> Events<D,S>
{
    using REACT_IMPL::MonitorNode;

    return Events<D,S>(
        std::make_shared<MonitorNode<D, S>>(
            target.NodePtr()));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Pulse
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S,
    typename E
>
auto Pulse(const Events<D,E>& trigger, const Signal<D,S>& target)
    -> Events<D,S>
{
    using REACT_IMPL::PulseNode;

    return Events<D,S>(
        std::make_shared<PulseNode<D,S,E>>(
            target.NodePtr(), trigger.NodePtr()));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Changed
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S
>
auto Changed(const Signal<D,S>& target)
    -> Events<D,EventToken>
{
    return Monitor(target)
        .Transform([] (const S& v) { return EventToken::token; });
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ChangedTo
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename V,
    typename S = std::decay<V>::type
>
auto ChangedTo(const Signal<D,S>& target, V&& value)
    -> Events<D,EventToken>
{
    return Monitor(target)
        .Transform([=] (const S& v) { return v == value; })
        .Filter([] (bool v) { return v == true; })
        .Transform([=] (const S& v) { return EventToken::token; })
}

/******************************************/ REACT_END /******************************************/
