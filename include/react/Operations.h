
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/Defs.h"

#include <functional>
#include <memory>
#include <thread>
#include <type_traits>
#include <utility>

#include "Signal.h"
#include "EventStream.h"
#include "react/graph/ConversionNodes.h"

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Fold
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename V,
    typename E,
    typename FIn,
    typename S = std::decay<V>::type,
    typename F = std::decay<FIn>::type
>
auto Fold(V&& init, const Events<D,E>& events, FIn&& func)
    -> Signal<D,S>
{
    return Signal<D,S>(
        std::make_shared<REACT_IMPL::FoldNode<D,S,E,F>>(
            std::forward<V>(init), events.GetPtr(), std::forward<FIn>(func)));
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
    typename S = std::decay<V>::type,
    typename F = std::decay<FIn>::type
>
auto Iterate(V&& init, const Events<D,E>& events, FIn&& func)
    -> Signal<D,S>
{
    return Signal<D,S>(
        std::make_shared<REACT_IMPL::IterateNode<D,S,E,F>>(
            std::forward<V>(init), events.GetPtr(), std::forward<FIn>(func)));
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
    return Signal<D,T>(
        std::make_shared<REACT_IMPL::HoldNode<D,T>>(
            std::forward<V>(init), events.GetPtr()));
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
auto Snapshot(const Signal<D,S>& target, const Events<D,E>& trigger)
    -> Signal<D,S>
{
    return Signal<D,S>(
        std::make_shared<REACT_IMPL::SnapshotNode<D,S,E>>(
            target.GetPtr(), trigger.GetPtr()));
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
    return Events<D,S>(
        std::make_shared<REACT_IMPL::MonitorNode<D, S>>(
            target.GetPtr()));
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
    -> Events<D,bool>
{
    return Transform(Monitor(target), [] (const S& v) { return true; });
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
    -> Events<D,bool>
{
    auto transformFunc  = [=] (const S& v)    { return v == value; };
    auto filterFunc     = [=] (bool v)        { return v == true; };

    return Filter(Transform(Monitor(target), transformFunc), filterFunc);
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
auto Pulse(const Signal<D,S>& target, const Events<D,E>& trigger)
    -> Events<D,S>
{
    return Events<D,S>(
        std::make_shared<REACT_IMPL::PulseNode<D,S,E>>(
            target.GetPtr(), trigger.GetPtr()));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Flatten
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename TInnerValue
>
auto Flatten(const Signal<D,Events<D,TInnerValue>>& node)
    -> Events<D,TInnerValue>
{
    return Events<D,TInnerValue>(
        std::make_shared<REACT_IMPL::EventFlattenNode<D, Events<D,TInnerValue>, TInnerValue>>(
            node.GetPtr(), node().GetPtr()));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Incrementer
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct Incrementer : public std::unary_function<T,T>
{
    T operator() (T v) const { return v+1; }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Decrementer
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct Decrementer : public std::unary_function<T,T>
{
    T operator() (T v) const { return v-1; }
};

/******************************************/ REACT_END /******************************************/
