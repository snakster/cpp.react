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
	typename D,
	typename E
>
class REvents : public Reactive<EventStreamNode<D,E>>
{
private:
	typedef EventStreamNode<D, E> NodeT;

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
namespace impl
{

template <typename D, typename L, typename R>
bool Equals(const REvents<D,L>& lhs, const REvents<D,R>& rhs)
{
	return lhs.Equals(rhs);
}

} // ~namespace react::impl

////////////////////////////////////////////////////////////////////////////////////////
/// REventSource
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename E
>
class REventSource : public REvents<D,E>
{
private:
	typedef EventSourceNode<D, E> NodeT;

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
template <typename D, typename E>
inline auto MakeEventSource()
	-> REventSource<D,E>
{
	return REventSource<D,E>(
		std::make_shared<EventSourceNode<D,E>>(false));
}

////////////////////////////////////////////////////////////////////////////////////////
/// Merge
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename TArg1,
	typename TArg2,
	typename ... TArgs
>
inline auto Merge(const REvents<D,TArg1>& arg1,
				  const REvents<D,TArg2>& arg2,
				  const REvents<D,TArgs>& ... args)
	-> REvents<D,TArg1>
{
	typedef TArg1 E;
	return REvents<D,E>(
		std::make_shared<EventMergeNode<D, E, TArg1, TArg2, TArgs ...>>(
			arg1.GetPtr(), arg2.GetPtr(), args.GetPtr() ..., false));
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
inline auto Filter(const REvents<D,E>& src, const F& filter)
	-> REvents<D,E>
{
	return REvents<D,E>(
		std::make_shared<EventFilterNode<D, E, F>>(src.GetPtr(), filter, false));
}

////////////////////////////////////////////////////////////////////////////////////////
/// Comparison operators
////////////////////////////////////////////////////////////////////////////////////////
#define DECLARE_COMP_OP(op)												\
																		\
template																\
<																		\
	typename D,															\
	typename E															\
>																		\
inline auto operator ## op(const REvents<D,E>& lhs, const E& rhs)		\
	-> REvents<D,E>														\
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
	typename D,
	typename TIn,
	typename F
>
inline auto Transform(const REvents<D,TIn>& src, const F& func)
	-> REvents<D, decltype(func(src.GetPtr()->Front()))>
{
	typedef decltype(func(src.GetPtr()->Front())) TOut;

	return REvents<D,TOut>(
		std::make_shared<EventTransformNode<D, TIn, TOut, F>>(
			src.GetPtr(), func, false));
}

} // ~namespace react