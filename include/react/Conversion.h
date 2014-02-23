#pragma once

#include <functional>
#include <memory>
#include <thread>

#include "Signal.h"
#include "EventStream.h"
#include "react/graph/ConversionNodes.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace react {

////////////////////////////////////////////////////////////////////////////////////////
/// Fold
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename E,
	typename S,
	typename TFunc,
	typename ... TArgs
>
inline auto Fold(const S& initialValue, const REvents<TDomain,E>& events,
				 const TFunc& func, const RSignal<TDomain,TArgs>& ... args)
	-> RSignal<TDomain,S>
{
	return RSignal<TDomain,S>(
		std::make_shared<FoldNode<TDomain,S,E,TArgs ...>>(
			initialValue, events.GetPtr(), args.GetPtr() ..., func, false));
}

////////////////////////////////////////////////////////////////////////////////////////
/// Iterate
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename E,
	typename S,
	typename TFunc,
	typename ... TArgs
>
inline auto Iterate(const S& initialValue, const REvents<TDomain,E>& events,
					const TFunc& func, const RSignal<TDomain,TArgs>& ... args)
	-> RSignal<TDomain,S>
{
	return RSignal<TDomain,S>(
		std::make_shared<IterateNode<TDomain,S,E,TArgs ...>>(
			initialValue, events.GetPtr(), args.GetPtr() ..., func, false));
}

////////////////////////////////////////////////////////////////////////////////////////
/// Hold
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename T
>
inline auto Hold(const T& initialValue, const REvents<TDomain,T>& events)
	-> RSignal<TDomain,T>
{
	return RSignal<TDomain,T>(
		std::make_shared<HoldNode<TDomain,T>>(
			initialValue, events.GetPtr(), false));
}

////////////////////////////////////////////////////////////////////////////////////////
/// Snapshot
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename S,
	typename E
>
inline auto Snapshot(const RSignal<TDomain,S>& target, const REvents<TDomain,E>& trigger)
	-> RSignal<TDomain,S>
{
	return RSignal<TDomain,S>(
		std::make_shared<SnapshotNode<TDomain,S,E>>(
			target.GetPtr(), trigger.GetPtr(), false));
}

template
<
	typename TDomain,
	typename S,
	typename E
>
inline auto operator&(const REvents<TDomain,E>& trigger,
					  const RSignal<TDomain,S>& target)
	-> RSignal<TDomain,S>
{
	return Snapshot(target,trigger);
}

////////////////////////////////////////////////////////////////////////////////////////
/// Monitor
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename S
>
inline auto Monitor(const RSignal<TDomain,S>& target)
	-> REvents<TDomain,S>
{
	return REvents<TDomain,S>(
		std::make_shared<MonitorNode<TDomain, S>>(
			target.GetPtr(), false));
}

////////////////////////////////////////////////////////////////////////////////////////
/// Changed
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename S
>
inline auto Changed(const RSignal<TDomain,S>& target)
	-> REvents<TDomain,bool>
{
	return Transform(Monitor(target), [] (const S& v) { return true; });
}

////////////////////////////////////////////////////////////////////////////////////////
/// ChangedTo
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename S
>
inline auto ChangedTo(const RSignal<TDomain,S>& target, const S& value)
	-> REvents<TDomain,bool>
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
	typename TDomain,
	typename S,
	typename E
>
inline auto Pulse(const RSignal<TDomain,S>& target, const REvents<TDomain,E>& trigger)
	-> REvents<TDomain,S>
{
	return REvents<TDomain,S>(
		std::make_shared<PulseNode<TDomain,S,E>>(
			target.GetPtr(), trigger.GetPtr(), false));
}

////////////////////////////////////////////////////////////////////////////////////////
/// Flatten
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	template <typename Domain_, typename Val_> class THandle,
	typename TInnerValue,
	class = std::enable_if<std::is_base_of<
		REvents<TDomain,TInnerValue>,
		THandle<TDomain,TInnerValue>>::value>::type
>
inline auto Flatten(const RSignal<TDomain,THandle<TDomain,TInnerValue>>& node)
	-> REvents<TDomain,TInnerValue>
{
	return REvents<TDomain,TInnerValue>(
		std::make_shared<EventFlattenNode<TDomain, REvents<TDomain,TInnerValue>, TInnerValue>>(
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

} //~namespace react