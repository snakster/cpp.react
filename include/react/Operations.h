
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

////////////////////////////////////////////////////////////////////////////////////////
REACT_BEGIN

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
inline auto Fold(V&& init, const REvents<D,E>& events, F&& func)
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
inline auto Iterate(V&& init, const REvents<D,E>& events, F&& func)
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
inline auto Hold(V&& init, const REvents<D,T>& events)
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
inline auto Snapshot(const RSignal<D,S>& target, const REvents<D,E>& trigger)
	-> RSignal<D,S>
{
	return RSignal<D,S>(
		std::make_shared<REACT_IMPL::SnapshotNode<D,S,E>>(
			target.GetPtr(), trigger.GetPtr(), false));
}

template
<
	typename D,
	typename S,
	typename E
>
inline auto operator&(const REvents<D,E>& trigger,
					  const RSignal<D,S>& target)
	-> RSignal<D,S>
{
	return Snapshot(target,trigger);
}

////////////////////////////////////////////////////////////////////////////////////////
/// Monitor
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename S
>
inline auto Monitor(const RSignal<D,S>& target)
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
inline auto Changed(const RSignal<D,S>& target)
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
inline auto ChangedTo(const RSignal<D,S>& target, V&& value)
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
inline auto Pulse(const RSignal<D,S>& target, const REvents<D,E>& trigger)
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
inline auto Flatten(const RSignal<D,THandle<D,TInnerValue>>& node)
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

REACT_END