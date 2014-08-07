
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
            std::forward<V>(init), GetNodePtr(events)));
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
            GetNodePtr(target)));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Iterate - Iteratively combines signal value with values from event stream (aka Fold)
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
    using REACT_IMPL::AddIterateRangeWrapper;
    using REACT_IMPL::AddIterateByRefRangeWrapper;
    using REACT_IMPL::IsCallableWith;
    using REACT_IMPL::EventRange;

    using F = typename std::decay<FIn>::type;

    using NodeT =
        typename std::conditional<
            IsCallableWith<F,S,EventRange<E>,S>::value,
            IterateNode<D,S,E,F>,
            typename std::conditional<
                IsCallableWith<F,S,E,S>::value,
                IterateNode<D,S,E,AddIterateRangeWrapper<E,S,F>>,
                typename std::conditional<
                    IsCallableWith<F, void, EventRange<E>, S&>::value,
                    IterateByRefNode<D,S,E,F>,
                    typename std::conditional<
                        IsCallableWith<F,void,E,S&>::value,
                        IterateByRefNode<D,S,E,AddIterateByRefRangeWrapper<E,S,F>>,
                        void
                    >::type
                >::type
            >::type
        >::type;

    static_assert(
        ! std::is_same<NodeT,void>::value,
        "Iterate: Passed function does not match any of the supported signatures.");

    return Signal<D,S>(
        std::make_shared<NodeT>(
            std::forward<V>(init), GetNodePtr(events), std::forward<FIn>(func)));
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
    using REACT_IMPL::AddIterateRangeWrapper;
    using REACT_IMPL::AddIterateByRefRangeWrapper;
    using REACT_IMPL::IsCallableWith;
    using REACT_IMPL::EventRange;

    using F = typename std::decay<FIn>::type;

    using NodeT =
        typename std::conditional<
            IsCallableWith<F,S,EventRange<E>,S,TDepValues ...>::value,
            SyncedIterateNode<D,S,E,F,TDepValues ...>,
            typename std::conditional<
                IsCallableWith<F,S,E,S,TDepValues ...>::value,
                SyncedIterateNode<D,S,E,
                    AddIterateRangeWrapper<E,S,F,TDepValues ...>,
                    TDepValues ...>,
                typename std::conditional<
                    IsCallableWith<F,void,EventRange<E>,S&,TDepValues ...>::value,
                    SyncedIterateByRefNode<D,S,E,F,TDepValues ...>,
                    typename std::conditional<
                        IsCallableWith<F,void,E,S&,TDepValues ...>::value,
                        SyncedIterateByRefNode<D,S,E,
                            AddIterateByRefRangeWrapper<E,S,F,TDepValues ...>,
                            TDepValues ...>,
                        void
                    >::type
                >::type
            >::type
        >::type;

    static_assert(
        ! std::is_same<NodeT,void>::value,
        "Iterate: Passed function does not match any of the supported signatures.");

    //static_assert(NodeT::dummy_error, "DUMP MY TYPE" );

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
                std::make_shared<NodeT>(
                    std::forward<V>(MyInit), GetNodePtr(MySource),
                    std::forward<FIn>(MyFunc), GetNodePtr(deps) ...));
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
            GetNodePtr(target), GetNodePtr(trigger)));
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
            GetNodePtr(target), GetNodePtr(trigger)));
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