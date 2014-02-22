#pragma once

#include <vector>
#include <thread>

#include "ReactiveBase.h"
#include "ReactiveDomain.h"
#include "react/graph/EventStreamNodes.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace react {

template <typename T>
class Reactive_;

////////////////////////////////////////////////////////////////////////////////////////
/// Events_
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename E
>
class Events_ : public Reactive_<EventStreamNode<TDomain,E>>
{
private:
	typedef EventStreamNode<TDomain, E> NodeT;

public:
	Events_() :
		Reactive_()
	{
	}

	explicit Events_(const std::shared_ptr<NodeT>& ptr) :
		Reactive_(ptr)
	{
	}
};

////////////////////////////////////////////////////////////////////////////////////////
/// EventSource_
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename E
>
class EventSource_ : public Events_<TDomain,E>
{
private:
	typedef EventSourceNode<TDomain, E> NodeT;

public:
	EventSource_() :
		Events_()
	{
	}

	explicit EventSource_(const std::shared_ptr<NodeT>& ptr) :
		Events_(ptr)
	{
	}

	EventSource_& operator<<(const E& e)
	{
		if (! Domain::TransactionInputContinuation::IsNull())
		{
			auto sourceNode = std::static_pointer_cast<NodeT>(ptr_);
			Domain::TransactionInputContinuation::Get()->AddEventInput_Safe(*sourceNode, e);
		}
		else if (! Domain::ScopedTransactionInput::IsNull())
		{
			auto sourceNode = std::static_pointer_cast<NodeT>(ptr_);
			Domain::ScopedTransactionInput::Get()->AddEventInput(*sourceNode, e);
		}
		else
		{
			auto sourceNode = std::static_pointer_cast<NodeT>(ptr_);
			Domain::Transaction t;
			t.Data().Input().AddEventInput(*sourceNode, e);
			t.Commit();
		}
		return *this;
	}
};

////////////////////////////////////////////////////////////////////////////////////////
/// MakeEventSource
////////////////////////////////////////////////////////////////////////////////////////
template <typename TDomain, typename E>
inline auto MakeEventSource()
	-> EventSource_<TDomain,E>
{
	return EventSource_<TDomain,E>(
		std::make_shared<EventSourceNode<TDomain,E>>(false));
}

////////////////////////////////////////////////////////////////////////////////////////
/// Merge
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename TArg1,
	typename TArg2,
	typename ... TArgs
>
inline auto Merge(const Events_<TDomain,TArg1>& arg1,
				  const Events_<TDomain,TArg2>& arg2,
				  const Events_<TDomain,TArgs>& ... args)
	-> Events_<TDomain,TArg1>
{
	typedef TArg1 E;
	return Events_<TDomain,E>(
		std::make_shared<EventMergeNode<TDomain, E, TArg1, TArg2, TArgs ...>>(
			arg1.GetPtr(), arg2.GetPtr(), args.GetPtr() ..., false));
}

template
<
	typename TDomain,
	typename TLeftArg,
	typename TRightArg
>
inline auto operator|(const Events_<TDomain,TLeftArg>& lhs,
					  const Events_<TDomain,TRightArg>& rhs)
	-> Events_<TDomain, TLeftArg>
{
	return Merge(lhs,rhs);
}

////////////////////////////////////////////////////////////////////////////////////////
/// Filter
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename E,
	typename F
>
inline auto Filter(const Events_<TDomain,E>& src, const F& filter)
	-> Events_<TDomain,E>
{
	return Events_<TDomain,E>(
		std::make_shared<EventFilterNode<TDomain, E, F>>(src.GetPtr(), filter, false));
}

////////////////////////////////////////////////////////////////////////////////////////
/// Comparison operators
////////////////////////////////////////////////////////////////////////////////////////
#define DECLARE_COMP_OP(op)												\
																		\
template																\
<																		\
	typename TDomain,													\
	typename E															\
>																		\
inline auto operator ## op(const Events_<TDomain,E>& lhs, const E& rhs)	\
	-> Events_<TDomain,E>												\
{																		\
	return Filter(lhs, [=] (const E& e) { return e op rhs; });			\
}

DECLARE_COMP_OP(==);
DECLARE_COMP_OP(!=);
DECLARE_COMP_OP(<);
DECLARE_COMP_OP(<=);
DECLARE_COMP_OP(>);
DECLARE_COMP_OP(>=);

#undef DECLARE_COMP_OP

////////////////////////////////////////////////////////////////////////////////////////
/// Transform
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename TIn,
	typename F
>
inline auto Transform(const Events_<TDomain,TIn>& src, const F& func)
	-> Events_<TDomain, decltype(func(src.GetPtr()->Front()))>
{
	typedef decltype(func(src.GetPtr()->Front())) TOut;

	return Events_<TDomain,TOut>(
		std::make_shared<EventTransformNode<TDomain, TIn, TOut, F>>(
			src.GetPtr(), func, false));
}

// ---
}