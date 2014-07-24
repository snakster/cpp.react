
//          Copyright Sebastian Jeckel 2014.
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

enum class Token;

template <typename D, typename ... TValues>
class SignalPack;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Hold - Hold the most recent event in a signal
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename V,
    typename T = typename std::decay<V>::type
>
auto Hold(const Events<D,T>& events, V&& init)
    -> Signal<D,T>
{
    using REACT_IMPL::HoldNode;

    return Signal<D,T>(
        std::make_shared<HoldNode<D,T>>(
            std::forward<V>(init), events.NodePtr()));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Monitor - Emits value changes of target signal
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
/// Iterate - Iteratively combines signal value with values from event stream
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename E,
    typename V,
    typename FIn,
    typename S = typename std::decay<V>::type
>
auto Iterate(const Events<D,E>& events, V&& init, FIn&& func)
    -> Signal<D,S>
{
    using REACT_IMPL::IterateNode;
    using REACT_IMPL::IterateByRefNode;

    using F = typename std::decay<FIn>::type;
    using R = typename std::result_of<FIn(E,S&)>::type;
    using TNode = typename std::conditional<
        std::is_same<void,R>::value,
        IterateByRefNode<D,S,E,F>,
        IterateNode<D,S,E,F>
            >::type;

    return Signal<D,S>(
        std::make_shared<TNode>(
            std::forward<V>(init), events.NodePtr(), std::forward<FIn>(func)));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Iterate - Synced
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename E,
    typename V,
    typename FIn,
    typename ... TDepValues,
    typename S = typename std::decay<V>::type
>
auto Iterate(const Events<D,E>& events, V&& init,
             const SignalPack<D,TDepValues...>& depPack, FIn&& func)
    -> Signal<D,S>
{
    using REACT_IMPL::SyncedIterateNode;
    using REACT_IMPL::SyncedIterateByRefNode;

    using F = typename std::decay<FIn>::type;
    using R = typename std::result_of<FIn(E,S&,TDepValues...)>::type;
    using TNode = typename std::conditional<
        std::is_same<void,R>::value,
        SyncedIterateByRefNode<D,S,E,F,TDepValues ...>,
        SyncedIterateNode<D,S,E,F,TDepValues ...>
            >::type;

    struct NodeBuilder_
    {
        NodeBuilder_(const Events<D,E>& source, V&& init, FIn&& func) :
            MySource( source ),
            MyInit( std::forward<V>(init) ),
            MyFunc( std::forward<FIn>(func) )
        {}

        auto operator()(const Signal<D,TDepValues>& ... deps)
            -> Signal<D,S>
        {
            return Signal<D,S>(
                std::make_shared<TNode>(
                    std::forward<V>(MyInit), MySource.NodePtr(),
                    std::forward<FIn>(MyFunc), deps.NodePtr() ...));
        }

        const Events<D,E>&  MySource;
        V                   MyInit;
        FIn                 MyFunc;
    };

    return REACT_IMPL::apply(
        NodeBuilder_( events, std::forward<V>(init), std::forward<FIn>(func) ),
        depPack.Data);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Snapshot - Sets signal value to value of other signal when event is received
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
/// Pulse - Emits value of target signal when event is received
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
/// Changed - Emits token when target signal was changed
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S
>
auto Changed(const Signal<D,S>& target)
    -> Events<D,Token>
{
    return Monitor(target).Tokenize();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ChangedTo - Emits token when target signal was changed to value
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename V,
    typename S = typename std::decay<V>::type
>
auto ChangedTo(const Signal<D,S>& target, V&& value)
    -> Events<D,Token>
{
    return Monitor(target)
        .Filter([=] (const S& v) { return v == value; })
        .Tokenize();
}

/******************************************/ REACT_END /******************************************/

#endif // REACT_ALGORITHM_H_INCLUDED