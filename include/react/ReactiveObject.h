
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/Defs.h"

#include "ReactiveDomain.h"

/***********************************/ REACT_BEGIN /************************************/

////////////////////////////////////////////////////////////////////////////////////////
/// ReactiveObject
////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class ReactiveObject
{
public:
	////////////////////////////////////////////////////////////////////////////////////////
	/// Aliases
	////////////////////////////////////////////////////////////////////////////////////////
	using DomainT = D;

	template <typename S>
	using Signal = RSignal<D,S>;

	template <typename S>
	using VarSignal = RVarSignal<D,S>;

	template <typename S>
	using RefSignal = RRefSignal<D,S>;

	template <typename S>
	using VarRefSignal = RVarRefSignal<D,S>;

	template <typename E = EventToken>
	using Events = REvents<D,E>;

	template <typename E = EventToken>
	using EventSource = REventSource<D,E>;

	using Observer = RObserver<D>;

	using ReactiveLoop = RReactiveLoop<D>;

	////////////////////////////////////////////////////////////////////////////////////////
	/// MakeVar
	////////////////////////////////////////////////////////////////////////////////////////
	template
	<
		typename V,
		typename S = std::decay<V>::type,
		class = std::enable_if<
			!IsSignal<D,S>::value>::type
	>
	static auto MakeVar(V&& value)
		-> VarSignal<S>
	{
		return react::MakeVar<D>(std::forward<V>(value));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// MakeVar (higher order signal)
	//////////////////////////////////////////////////////////////////////////////////////
	template
	<
		typename V,
		typename S = std::decay<V>::type,
		typename TInner = S::ValueT,
		class = std::enable_if<
			IsSignal<D,S>::value>::type
	>
	static auto MakeVar(V&& value)
		-> VarSignal<Signal<TInner>>
	{
		return react::MakeVar<D>(std::forward<V>(value));
	}

	////////////////////////////////////////////////////////////////////////////////////////
	/// MakeSignal
	////////////////////////////////////////////////////////////////////////////////////////
	template
	<
		typename F,
		typename ... TArgs
	>
	static auto MakeSignal(F&& func, const Signal<TArgs>& ... args)
		-> Signal<typename std::result_of<F(TArgs...)>::type>
	{
		using S = typename std::result_of<F(TArgs...)>::type;

		return react::MakeSignal<D>(std::forward<F>(func), args ...);
	}

	////////////////////////////////////////////////////////////////////////////////////////
	/// MakeEventSource
	////////////////////////////////////////////////////////////////////////////////////////
	template <typename E>
	static auto MakeEventSource()
		-> EventSource<E>
	{
		return react::MakeEventSource<D,E>();
	}

	////////////////////////////////////////////////////////////////////////////////////////
	/// Flatten macros
	////////////////////////////////////////////////////////////////////////////////////////
	// Todo: Add safety wrapper + static assert to check for this for ReactiveObject
	#define REACTIVE_REF(obj, name)											\
		Flatten(															\
			MakeSignal(														\
				[] (REACT_IMPL::Identity<decltype(obj)>::Type::ValueT::type r) \
				{															\
					return static_cast<RemoveInput<DomainT, decltype(r.name)>::Type>(r.name); \
                },															\
				obj))

	#define REACTIVE_PTR(obj, name)											\
		Flatten(															\
			MakeSignal(														\
				[] (REACT_IMPL::Identity<decltype(obj)>::Type::ValueT r)	\
				{															\
					REACT_ASSERT(r != nullptr);								\
					return static_cast<RemoveInput<DomainT, decltype(r->name)>::Type>(r->name); \
				},															\
				obj))
};

/************************************/ REACT_END /*************************************/
