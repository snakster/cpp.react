#pragma once

#include <vector>
#include <thread>

#include "ReactiveBase.h"
#include "ReactiveDomain.h"
#include "react/graph/EventStreamNodes.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace react {

template <typename T>
class Reactive;

////////////////////////////////////////////////////////////////////////////////////////
/// REvents
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename E
>
class REvents : public Reactive<EventStreamNode<TDomain,E>>
{
private:
	typedef EventStreamNode<TDomain, E> NodeT;

public:
	REvents() :
		Reactive()
	{
	}

	explicit REvents(const std::shared_ptr<NodeT>& ptr) :
		Reactive(ptr)
	{
	}
};

////////////////////////////////////////////////////////////////////////////////////////
/// REventSource
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename E
>
class REventSource : public REvents<TDomain,E>
{
private:
	typedef EventSourceNode<TDomain, E> NodeT;

public:
	REventSource() :
		REvents()
	{
	}

	explicit REventSource(const std::shared_ptr<NodeT>& ptr) :
		REvents(ptr)
	{
	}

	REventSource& operator<<(const E& e)
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
	-> REventSource<TDomain,E>
{
	return REventSource<TDomain,E>(
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
inline auto Merge(const REvents<TDomain,TArg1>& arg1,
				  const REvents<TDomain,TArg2>& arg2,
				  const REvents<TDomain,TArgs>& ... args)
	-> REvents<TDomain,TArg1>
{
	typedef TArg1 E;
	return REvents<TDomain,E>(
		std::make_shared<EventMergeNode<TDomain, E, TArg1, TArg2, TArgs ...>>(
			arg1.GetPtr(), arg2.GetPtr(), args.GetPtr() ..., false));
}

template
<
	typename TDomain,
	typename TLeftArg,
	typename TRightArg
>
inline auto operator|(const REvents<TDomain,TLeftArg>& lhs,
					  const REvents<TDomain,TRightArg>& rhs)
	-> REvents<TDomain, TLeftArg>
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
inline auto Filter(const REvents<TDomain,E>& src, const F& filter)
	-> REvents<TDomain,E>
{
	return REvents<TDomain,E>(
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
inline auto operator ## op(const REvents<TDomain,E>& lhs, const E& rhs)	\
	-> REvents<TDomain,E>												\
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
inline auto Transform(const REvents<TDomain,TIn>& src, const F& func)
	-> REvents<TDomain, decltype(func(src.GetPtr()->Front()))>
{
	typedef decltype(func(src.GetPtr()->Front())) TOut;

	return REvents<TDomain,TOut>(
		std::make_shared<EventTransformNode<TDomain, TIn, TOut, F>>(
			src.GetPtr(), func, false));
}

// ---
}