
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/Defs.h"

#include <thread>
#include <type_traits>
#include <vector>

#include "ReactiveBase.h"
#include "ReactiveDomain.h"
#include "Observer.h"
#include "react/graph/EventStreamNodes.h"

////////////////////////////////////////////////////////////////////////////////////////
REACT_BEGIN

template <typename T>
class Reactive;

enum class EventToken
{
	token
};

////////////////////////////////////////////////////////////////////////////////////////
/// REvents
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename E = EventToken
>
class REvents : public Reactive<REACT_IMPL::EventStreamNode<D,E>>
{
protected:
	using NodeT = REACT_IMPL::EventStreamNode<D, E>;

public:
	using ValueT = E;

	REvents() :
		Reactive()
	{
	}

	explicit REvents(const std::shared_ptr<NodeT>& ptr) :
		Reactive(ptr)
	{
	}

	template <typename F>
	REvents Filter(F&& f)
	{
		return react::Filter(*this, std::forward<F>(f));
	}

	template <typename F>
	REvents Transform(F&& f)
	{
		return react::Transform(*this, std::forward<F>(f));
	}

	template <typename F>
	RObserver<D> Observe(F&& f)
	{
		return react::Observe(*this, std::forward<F>(f));
	}
};

REACT_END

REACT_IMPL_BEGIN

template <typename D, typename L, typename R>
bool Equals(const REvents<D,L>& lhs, const REvents<D,R>& rhs)
{
	return lhs.Equals(rhs);
}

REACT_IMPL_END

REACT_BEGIN

////////////////////////////////////////////////////////////////////////////////////////
/// REventSource
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename E = EventToken
>
class REventSource : public REvents<D,E>
{
private:
	using NodeT = REACT_IMPL::EventSourceNode<D, E>;

public:
	REventSource() :
		REvents()
	{
	}

	explicit REventSource(const std::shared_ptr<NodeT>& ptr) :
		REvents(ptr)
	{
	}

	template <typename V>
	void Emit(V&& v) const
	{
		D::AddInput(*std::static_pointer_cast<NodeT>(ptr_), std::forward<V>(v));
	}

	template <typename = std::enable_if<std::is_same<E,EventToken>::value>::type>
	void Emit() const
	{
		Emit(EventToken::token);
	}

	const REventSource& operator<<(const E& e) const
	{
		Emit(e);
		return *this;
	}
};

////////////////////////////////////////////////////////////////////////////////////////
/// IsEventT
////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename T>
struct IsEventT { static const bool value = false; };

template <typename D, typename T>
struct IsEventT<D, REvents<D,T>> { static const bool value = true; };

template <typename D, typename T>
struct IsEventT<D, REventSource<D,T>> { static const bool value = true; };

////////////////////////////////////////////////////////////////////////////////////////
/// MakeEventSource
////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename E>
inline auto MakeEventSource()
	-> REventSource<D,E>
{
	return REventSource<D,E>(
		std::make_shared<REACT_IMPL::EventSourceNode<D,E>>(false));
}

template <typename D>
inline auto MakeEventSource()
	-> REventSource<D,EventToken>
{
	return REventSource<D,EventToken>(
		std::make_shared<REACT_IMPL::EventSourceNode<D,EventToken>>(false));
}

////////////////////////////////////////////////////////////////////////////////////////
/// Merge
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename TArg1,
	typename ... TArgs
>
inline auto Merge(const REvents<D,TArg1>& arg1,
				  const REvents<D,TArgs>& ... args)
	-> REvents<D,TArg1>
{
	static_assert(sizeof...(TArgs) > 0,
		"react::Merge requires at least 2 arguments.");

	typedef TArg1 E;
	return REvents<D,E>(
		std::make_shared<REACT_IMPL::EventMergeNode<D, E, TArg1, TArgs ...>>(
			arg1.GetPtr(), args.GetPtr() ..., false));
}

template
<
	typename D,
	typename TLeftArg,
	typename TRightArg
>
inline auto operator|(const REvents<D,TLeftArg>& lhs,
					  const REvents<D,TRightArg>& rhs)
	-> REvents<D, TLeftArg>
{
	return Merge(lhs,rhs);
}

////////////////////////////////////////////////////////////////////////////////////////
/// Filter
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename E,
	typename F
>
inline auto Filter(const REvents<D,E>& src, F&& filter)
	-> REvents<D,E>
{
	return REvents<D,E>(
		std::make_shared<REACT_IMPL::EventFilterNode<D, E>>(
			src.GetPtr(), std::forward<F>(filter), false));
}

////////////////////////////////////////////////////////////////////////////////////////
/// Comparison operators
/// TODO: Bad idea...
////////////////////////////////////////////////////////////////////////////////////////
//#define DECLARE_COMP_OP(op)												\
//																		\
//template																\
//<																		\
//	typename D,															\
//	typename E															\
//>																		\
//inline auto operator ## op(const REvents<D,E>& lhs, const E& rhs)		\
//	-> REvents<D,E>														\
//{																		\
//	return Filter(lhs, [=] (const E& e) { return e op rhs; });			\
//}
//
//DECLARE_COMP_OP(==);
//DECLARE_COMP_OP(!=);
//DECLARE_COMP_OP(<);
//DECLARE_COMP_OP(<=);
//DECLARE_COMP_OP(>);
//DECLARE_COMP_OP(>=);
//
//#undef DECLARE_COMP_OP

////////////////////////////////////////////////////////////////////////////////////////
/// Transform
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename TIn,
	typename F
>
inline auto Transform(const REvents<D,TIn>& src, F&& func)
	-> REvents<D, decltype(func(src.GetPtr()->Front()))>
{
	typedef decltype(func(src.GetPtr()->Front())) TOut;

	return REvents<D,TOut>(
		std::make_shared<REACT_IMPL::EventTransformNode<D, TIn, TOut>>(
			src.GetPtr(), std::forward<F>(func), false));
}

REACT_END