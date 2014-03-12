
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

/***********************************/ REACT_BEGIN /************************************/

////////////////////////////////////////////////////////////////////////////////////////
/// Fold
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename V,
	typename S = std::decay<V>::type,
	typename E,
	typename F
>
auto Fold(V&& init, const REvents<D,E>& events, F&& func)
	-> RSignal<D,S>
{
	return RSignal<D,S>(
		std::make_shared<REACT_IMPL::FoldNode<D,S,E>>(
			std::forward<V>(init), events.GetPtr(), std::forward<F>(func), false));
}

////////////////////////////////////////////////////////////////////////////////////////
/// Iterate
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename V,
	typename S = std::decay<V>::type,
	typename E,
	typename F
>
auto Iterate(V&& init, const REvents<D,E>& events, F&& func)
	-> RSignal<D,S>
{
	return RSignal<D,S>(
		std::make_shared<REACT_IMPL::IterateNode<D,S,E>>(
			std::forward<V>(init), events.GetPtr(), std::forward<F>(func), false));
}

////////////////////////////////////////////////////////////////////////////////////////
/// Hold
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename V,
	typename T = std::decay<V>::type
>
auto Hold(V&& init, const REvents<D,T>& events)
	-> RSignal<D,T>
{
	return RSignal<D,T>(
		std::make_shared<REACT_IMPL::HoldNode<D,T>>(
			std::forward<V>(init), events.GetPtr(), false));
}

////////////////////////////////////////////////////////////////////////////////////////
/// Snapshot
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename S,
	typename E
>
auto Snapshot(const RSignal<D,S>& target, const REvents<D,E>& trigger)
	-> RSignal<D,S>
{
	return RSignal<D,S>(
		std::make_shared<REACT_IMPL::SnapshotNode<D,S,E>>(
			target.GetPtr(), trigger.GetPtr(), false));
}

////////////////////////////////////////////////////////////////////////////////////////
/// Monitor
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename S
>
auto Monitor(const RSignal<D,S>& target)
	-> REvents<D,S>
{
	return REvents<D,S>(
		std::make_shared<REACT_IMPL::MonitorNode<D, S>>(
			target.GetPtr(), false));
}

////////////////////////////////////////////////////////////////////////////////////////
/// Changed
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename S
>
auto Changed(const RSignal<D,S>& target)
	-> REvents<D,bool>
{
	return Transform(Monitor(target), [] (const S& v) { return true; });
}

////////////////////////////////////////////////////////////////////////////////////////
/// ChangedTo
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename V,
	typename S = std::decay<V>::type
>
auto ChangedTo(const RSignal<D,S>& target, V&& value)
	-> REvents<D,bool>
{
	auto transformFunc	= [=] (const S& v)	{ return v == value; };
	auto filterFunc		= [=] (bool v)		{ return v == true; };

	return Filter(Transform(Monitor(target), transformFunc), filterFunc);
}

////////////////////////////////////////////////////////////////////////////////////////
/// Pulse
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename S,
	typename E
>
auto Pulse(const RSignal<D,S>& target, const REvents<D,E>& trigger)
	-> REvents<D,S>
{
	return REvents<D,S>(
		std::make_shared<REACT_IMPL::PulseNode<D,S,E>>(
			target.GetPtr(), trigger.GetPtr(), false));
}

////////////////////////////////////////////////////////////////////////////////////////
/// Flatten
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	template <typename Domain_, typename Val_> class THandle,
	typename TInnerValue,
	class = std::enable_if<std::is_base_of<
		REvents<D,TInnerValue>,
		THandle<D,TInnerValue>>::value>::type
>
auto Flatten(const RSignal<D,THandle<D,TInnerValue>>& node)
	-> REvents<D,TInnerValue>
{
	return REvents<D,TInnerValue>(
		std::make_shared<REACT_IMPL::EventFlattenNode<D, REvents<D,TInnerValue>, TInnerValue>>(
			node.GetPtr(), node().GetPtr(), false));
}

////////////////////////////////////////////////////////////////////////////////////////
/// Incrementer
////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct Incrementer : public std::unary_function<T,T>
{
	T operator() (T v) { return v+1; }
};

////////////////////////////////////////////////////////////////////////////////////////
/// Decrementer
////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct Decrementer : public std::unary_function<T,T>
{
	T operator() (T v) { return v-1; }
};

/************************************/ REACT_END /*************************************/
