#pragma once

#include <functional>
#include <memory>
#include <thread>
#include <type_traits>

#include "ReactiveBase.h"
#include "ReactiveDomain.h"

#include "react/common/Util.h"
#include "react/graph/SignalNodes.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace react {

template <typename D, typename S>
class SignalNode;

template <typename D, typename S>
class VarNode;

template
<
	typename D,
	typename S,
	typename ... TArgs
>
class FunctionNode;

////////////////////////////////////////////////////////////////////////////////////////
/// RSignal
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename S
>
class RSignal : public Reactive<SignalNode<D,S>>
{
protected:
	typedef SignalNode<D,S> NodeT;

public:
	typedef S ValueT;

	RSignal() :
		Reactive()
	{
	}
	
	explicit RSignal(const std::shared_ptr<NodeT>& ptr) :
		Reactive(ptr)
	{
	}

	explicit RSignal(std::shared_ptr<NodeT>&& ptr) :
		Reactive(std::move(ptr))
	{
	}

	const S Value() const
	{
		return (*ptr_)();
	}

	S operator()(void) const
	{
		return Value();
	}
};

////////////////////////////////////////////////////////////////////////////////////////
namespace impl
{

template <typename D, typename L, typename R>
bool Equals(const RSignal<D,L>& lhs, const RSignal<D,R>& rhs)
{
	return lhs.Equals(rhs);
}

} // ~namespace react::impl

////////////////////////////////////////////////////////////////////////////////////////
/// RVarSignal
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename S
>
class RVarSignal : public RSignal<D,S>
{
protected:
	typedef VarNode<D,S> NodeT;

public:	
	RVarSignal() :
		RSignal()
	{
	}
	
	explicit RVarSignal(const std::shared_ptr<NodeT>& ptr) :
		RSignal(ptr)
	{
	}

	explicit RVarSignal(std::shared_ptr<NodeT>&& ptr) :
		RSignal(std::move(ptr))
	{
	}

	RVarSignal& operator<<=(const S& newValue)
	{
		if (! Domain::TransactionInputContinuation::IsNull())
		{
			auto varNode = std::static_pointer_cast<NodeT>(ptr_);
			Domain::TransactionInputContinuation::Get()->AddSignalInput_Safe(*varNode, newValue);
		}
		else if (! Domain::ScopedTransactionInput::IsNull())
		{
			auto varNode = std::static_pointer_cast<NodeT>(ptr_);
			Domain::ScopedTransactionInput::Get()->AddSignalInput(*varNode, newValue);
		}
		else
		{
			auto varNode = std::static_pointer_cast<NodeT>(ptr_);
			Domain::Transaction t;
			t.Data().Input().AddSignalInput(*varNode, newValue);
			t.Commit();
		}
		return *this;
	}
};

////////////////////////////////////////////////////////////////////////////////////////
/// MakeVar (higher order signal)
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	template <typename Domain_, typename Val_> class TOuter,
	typename TInner
>
inline auto MakeVar(const TOuter<D,TInner>& value)
	-> RVarSignal<D,RSignal<D,TInner>>
{
	return RVarSignal<D,RSignal<D,TInner>>(
		std::make_shared<VarNode<D,RSignal<D,TInner>>>(
			value, false));
}

////////////////////////////////////////////////////////////////////////////////////////
/// MakeVar
////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename S>
inline auto MakeVar(const S& value)
	-> RVarSignal<D,S>
{
	return RVarSignal<D,S>(std::make_shared<VarNode<D,S>>(value, false));
}

////////////////////////////////////////////////////////////////////////////////////////
/// MakeVal
////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename S>
inline auto MakeVal(const S& value)
	-> RSignal<D,S>
{
	return RSignal<D,S>(std::make_shared<ValNode<D,S>>(value, false));
}

////////////////////////////////////////////////////////////////////////////////////////
/// MakeSignal
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename TFunc,
	typename ... TArgs
>
inline auto MakeSignal(TFunc func, const RSignal<D,TArgs>& ... args)
	-> RSignal<D,decltype(func(args() ...))>
{
	typedef decltype(func(args() ...)) S;

	return RSignal<D,S>(
		std::make_shared<FunctionNode<D, S, TArgs ...>>(
			args.GetPtr() ..., func, false));
}

////////////////////////////////////////////////////////////////////////////////////////
/// Unary arithmetic operators
////////////////////////////////////////////////////////////////////////////////////////
#define DECLARE_OP(op)												\
template															\
<																	\
	typename D,														\
	typename TVal													\
>																	\
inline auto operator ## op(const RSignal<D,TVal>& arg)				\
	-> RSignal<D,TVal>												\
{																	\
	return RSignal<D,TVal>(											\
		std::make_shared<FunctionNode<D,TVal,TVal>>(				\
			arg.GetPtr(), [] (TVal a) { return op a; }, false));	\
}

DECLARE_OP(+);
DECLARE_OP(-);

#undef DECLARE_OP

////////////////////////////////////////////////////////////////////////////////////////
/// Binary arithmetic operators
////////////////////////////////////////////////////////////////////////////////////////
#define DECLARE_OP(op)												\
template															\
<																	\
	typename D,														\
	template <typename D_, typename V_> class TLeftHandle,			\
	template <typename D_, typename V_> class TRightHandle,			\
	typename TLeftVal,												\
	typename TRightVal,												\
	class = std::enable_if<std::is_base_of<							\
		RSignal<D,TLeftVal>,										\
		TLeftHandle<D,TLeftVal>>::value>::type,						\
	class = std::enable_if<std::is_base_of<							\
		RSignal<D,TRightVal>,										\
		TRightHandle<D,TLeftVal>>::value>::type						\
>																	\
inline auto operator ## op(const TLeftHandle<D,TLeftVal>& lhs,		\
						   const TRightHandle<D,TRightVal>& rhs)	\
	-> RSignal<D,TLeftVal>											\
{																	\
	return RSignal<D,TLeftVal>(										\
		std::make_shared<FunctionNode<D,TLeftVal,TLeftVal,TRightVal>>( \
			lhs.GetPtr(), rhs.GetPtr(), [] (TLeftVal a, TRightVal b) { return a op b; }, false)); \
}																	\
																	\
template															\
<																	\
	typename D,														\
	template <typename D_, typename V_> class TLeftHandle,			\
	typename TLeftVal,												\
	typename TRightVal,												\
	class = std::enable_if<std::is_base_of<							\
		RSignal<D,TLeftVal>,										\
		TLeftHandle<D,TLeftVal>>::value>::type,						\
	class = std::enable_if<											\
		std::is_integral<TRightVal>::value>::type					\
>																	\
inline auto operator ## op(const TLeftHandle<D,TLeftVal>& lhs,		\
						   const TRightVal& rhs)					\
	-> RSignal<D,TLeftVal>											\
{																	\
	return RSignal<D,TLeftVal>(										\
		std::make_shared<FunctionNode<D,TLeftVal,TLeftVal>>(		\
			lhs.GetPtr(), [=] (TLeftVal a) { return a op rhs; }, false)); \
}																	\
																	\
template															\
<																	\
	typename D,														\
	template <typename D_, typename V_> class TRightHandle,			\
	typename TLeftVal,												\
	typename TRightVal,												\
	class = std::enable_if<std::is_base_of<							\
		RSignal<D,TRightVal>,										\
		TRightHandle<D,TRightVal>>::value>::type,					\
	class = std::enable_if<											\
		std::is_integral<TLeftVal>::value>::type					\
>																	\
inline auto operator ## op(const TLeftVal& lhs,						\
						   const TRightHandle<D,TRightVal>& rhs)	\
	-> RSignal<D,TRightVal>											\
{																	\
	return RSignal<D,TRightVal>(									\
		std::make_shared<FunctionNode<D,TRightVal,TRightVal>>(		\
			rhs.GetPtr(), [=] (TRightVal a) { return lhs op a; }, false)); \
}

DECLARE_OP(+);
DECLARE_OP(-);
DECLARE_OP(*);
DECLARE_OP(/);
DECLARE_OP(%);

#undef DECLARE_OP

////////////////////////////////////////////////////////////////////////////////////////
/// Comparison operators
////////////////////////////////////////////////////////////////////////////////////////
#define DECLARE_OP(op)												\
template															\
<																	\
	typename D,														\
	typename TLeftVal,												\
	typename TRightVal												\
>																	\
inline auto operator ## op(const RSignal<D,TLeftVal>& lhs,			\
						   const RSignal<D,TRightVal>& rhs)			\
	-> RSignal<D,bool>												\
{																	\
	return RSignal<D,bool>(											\
		std::make_shared<FunctionNode<D,bool,TLeftVal,TRightVal>>(	\
			lhs.GetPtr(), rhs.GetPtr(), [] (TLeftVal a, TRightVal b) { return a op b; }, false)); \
}																	\
																	\
template															\
<																	\
	typename D,														\
	typename TLeftVal,												\
	typename TRightVal												\
>																	\
inline auto operator ## op(const RSignal<D,TLeftVal>& lhs, const TRightVal& rhs) \
	-> RSignal<D,bool>												\
{																	\
	return RSignal<D,bool>(											\
		std::make_shared<FunctionNode<D,bool,TLeftVal>>(			\
			lhs.GetPtr(), [=] (TLeftVal a) { return a op rhs; }, false)); \
}

DECLARE_OP(==);
DECLARE_OP(!=);
DECLARE_OP(<);
DECLARE_OP(<=);
DECLARE_OP(>);
DECLARE_OP(>=);

#undef DECLARE_OP

////////////////////////////////////////////////////////////////////////////////////////
/// Unary logical operators
////////////////////////////////////////////////////////////////////////////////////////
#define DECLARE_OP(op)												\
template															\
<																	\
	typename D,														\
	typename TVal													\
>																	\
inline auto operator ## op(const RSignal<D,TVal>& arg)				\
	-> RSignal<D,bool>												\
{																	\
	return RSignal<D,TVal>(											\
		std::make_shared<FunctionNode<D,bool,TVal>>(				\
			arg.GetPtr(), [] (TVal a) { return op a; }, false));	\
}

DECLARE_OP(!);

#undef DECLARE_OP

////////////////////////////////////////////////////////////////////////////////////////
/// Binary logical operators
////////////////////////////////////////////////////////////////////////////////////////
#define DECLARE_OP(op)												\
template															\
<																	\
	typename D,														\
	typename TLeftVal,												\
	typename TRightVal												\
>																	\
inline auto operator ## op(const RSignal<D,TLeftVal>& lhs,			\
						   const RSignal<D,TRightVal>& rhs)			\
	-> RSignal<D,bool>												\
{																	\
	return RSignal<D,bool>(											\
		std::make_shared<FunctionNode<D,bool,TLeftVal,TRightVal>>(	\
			lhs.GetPtr(), rhs.GetPtr(), [] (TLeftVal a, TRightVal b) { return a op b; }, false)); \
}																	\
																	\
template															\
<																	\
	typename D,														\
	typename TLeftVal,												\
	typename TRightVal												\
>																	\
inline auto operator ## op(const RSignal<D,TLeftVal>& lhs, const TRightVal& rhs) \
	-> RSignal<D,bool>												\
{																	\
	return RSignal<D,bool>(											\
		std::make_shared<FunctionNode<D,bool,TLeftVal>>(			\
			lhs.GetPtr(), [=] (TLeftVal a) { return a op rhs; }, false)); \
}

DECLARE_OP(&&);
DECLARE_OP(||);

#undef DECLARE_OP

////////////////////////////////////////////////////////////////////////////////////////
/// InputPack - Wraps several nodes in a tuple. Create with comma operator.
////////////////////////////////////////////////////////////////////////////////////////
namespace impl {

template
<
	typename D,
	typename ... TValues
>
struct InputPack
{
	std::tuple<RSignal<D, TValues> ...> Data;

	template <typename TFirstValue, typename TSecondValue>
	InputPack(const RSignal<D,TFirstValue>& first, const RSignal<D,TSecondValue>& second) :
		Data(std::make_tuple(first, second))
	{
	}

	template <typename ... TCurValues, typename TAppendValue>
	InputPack(const InputPack<D, TCurValues ...>& curArgs, const RSignal<D,TAppendValue>& newArg) :
		Data(std::tuple_cat(curArgs.Data, std::make_tuple(newArg)))
	{
	}
};

} // ~namespace react::impl

////////////////////////////////////////////////////////////////////////////////////////
/// Comma operator overload to create input pack from 2 nodes.
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename TLeftVal,
	typename TRightVal
>
inline auto operator,(const RSignal<D,TLeftVal>& a, const RSignal<D,TRightVal>& b)
	-> impl::InputPack<D,TLeftVal, TRightVal>
{
	return impl::InputPack<D, TLeftVal, TRightVal>(a, b);
}

////////////////////////////////////////////////////////////////////////////////////////
/// Comma operator overload to append node to existing input pack.
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename ... TCurValues,
	typename TAppendValue
>
inline auto operator,(const impl::InputPack<D, TCurValues ...>& cur, const RSignal<D,TAppendValue>& append)
	-> impl::InputPack<D,TCurValues ... , TAppendValue>
{
	return impl::InputPack<D, TCurValues ... , TAppendValue>(cur, append);
}

////////////////////////////////////////////////////////////////////////////////////////
/// operator>>= overload to connect inputs to a function and return the resulting node.
////////////////////////////////////////////////////////////////////////////////////////

namespace impl {

template
<
	typename D,
	typename TFunc,
	typename ... TValues
>
struct ApplyHelper
{
	static inline auto MakeSignal(const TFunc& func, const RSignal<D,TValues>& ... args)
		-> decltype(D::MakeSignal(func, args ...))
	{
		return D::MakeSignal(func, args ...);
	}
};

} // ~namespace react::impl

// Single input
template
<
	typename D,
	typename TFunc,
	typename TValue
>
inline auto operator>>=(const RSignal<D,TValue>& inputNode, TFunc func)
	-> decltype(D::MakeSignal(func, inputNode))
{
	return D::MakeSignal(func, inputNode);
}

// Multiple inputs
template
<
	typename D,
	typename TFunc,
	typename ... TValues
>
inline auto operator>>=(impl::InputPack<D,TValues ...>& inputPack, TFunc func)
	-> decltype(apply(impl::ApplyHelper<D, TFunc, TValues ...>::MakeSignal, std::tuple_cat(std::make_tuple(func), inputPack.Data)))
{
	return apply(impl::ApplyHelper<D, TFunc, TValues ...>::MakeSignal, std::tuple_cat(std::make_tuple(func), inputPack.Data));
}

////////////////////////////////////////////////////////////////////////////////////////
/// Flatten
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename TInnerValue
>
inline auto Flatten(const RSignal<D,RSignal<D,TInnerValue>>& node)
	-> RSignal<D,TInnerValue>
{
	return RSignal<D,TInnerValue>(
		std::make_shared<FlattenNode<D, RSignal<D,TInnerValue>, TInnerValue>>(
			node.GetPtr(), node().GetPtr(), false));
}

} // ~namespace react