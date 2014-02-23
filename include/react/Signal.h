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

////////////////////////////////////////////////////////////////////////////////////////
/// Signal_
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename S
>
class Signal_ : public Reactive_<SignalNode<TDomain,S>>
{
protected:
	typedef SignalNode<TDomain,S> NodeT;

public:
	typedef S ValueT;

	Signal_() :
		Reactive_()
	{
	}
	
	explicit Signal_(const std::shared_ptr<NodeT>& ptr) :
		Reactive_(ptr)
	{
	}

	explicit Signal_(std::shared_ptr<NodeT>&& ptr) :
		Reactive_(std::move(ptr))
	{
	}

	const S Value() const
	{
		return (*ptr_)();
	}

	S operator()(void) const
	{
		return (*ptr_)();
	}
};

////////////////////////////////////////////////////////////////////////////////////////
/// VarSignal_
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename S
>
class VarSignal_ : public Signal_<TDomain,S>
{
protected:
	typedef VarNode<TDomain,S> NodeT;

public:	
	VarSignal_() :
		Signal_()
	{
	}
	
	explicit VarSignal_(const std::shared_ptr<NodeT>& ptr) :
		Signal_(ptr)
	{
	}

	explicit VarSignal_(std::shared_ptr<NodeT>&& ptr) :
		Signal_(std::move(ptr))
	{
	}

	VarSignal_& operator<<=(const S& newValue)
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
	typename TDomain,
	template <typename Domain_, typename Val_> class TOuter,
	typename TInner
>
inline auto MakeVar(const TOuter<TDomain,TInner>& value)
	-> VarSignal_<TDomain,Signal_<TDomain,TInner>>
{
	return VarSignal_<TDomain,Signal_<TDomain,TInner>>(
		std::make_shared<VarNode<TDomain,Signal_<TDomain,TInner>>>(
			value, false));
}

////////////////////////////////////////////////////////////////////////////////////////
/// MakeVar
////////////////////////////////////////////////////////////////////////////////////////
template <typename TDomain, typename S>
inline auto MakeVar(const S& value)
	-> VarSignal_<TDomain,S>
{
	return VarSignal_<TDomain,S>(std::make_shared<VarNode<TDomain,S>>(value, false));
}

////////////////////////////////////////////////////////////////////////////////////////
/// MakeVal
////////////////////////////////////////////////////////////////////////////////////////
template <typename TDomain, typename S>
inline auto MakeVal(const S& value)
	-> Signal_<TDomain,S>
{
	return Signal_<TDomain,S>(std::make_shared<ValNode<TDomain,S>>(value, false));
}

////////////////////////////////////////////////////////////////////////////////////////
/// MakeSignal
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename TFunc,
	typename ... TArgs
>
inline auto MakeSignal(TFunc func, const Signal_<TDomain,TArgs>& ... args)
	-> Signal_<TDomain,decltype(func(args() ...))>
{
	typedef decltype(func(args() ...)) S;

	return Signal_<TDomain,S>(
		std::make_shared<FunctionNode<TDomain, S, TArgs ...>>(
			args.GetPtr() ..., func, false));
}

////////////////////////////////////////////////////////////////////////////////////////
/// Unary arithmetic operators
////////////////////////////////////////////////////////////////////////////////////////
#define DECLARE_ARITHMETIC_OP1(op)									\
template															\
<																	\
	typename TDomain,												\
	typename TVal													\
>																	\
inline auto operator ## op(const Signal_<TDomain,TVal>& arg)		\
	-> Signal_<TDomain,TVal>										\
{																	\
	return Signal_<TDomain,TVal>(									\
		std::make_shared<FunctionNode<TDomain,TVal,TVal>>(			\
			arg.GetPtr(), [] (TVal a) { return op a; }, false));	\
}

DECLARE_ARITHMETIC_OP1(+);
DECLARE_ARITHMETIC_OP1(-);

#undef DECLARE_ARITHMETIC_OP1

////////////////////////////////////////////////////////////////////////////////////////
/// Binary arithmetic operators
////////////////////////////////////////////////////////////////////////////////////////
#define DECLARE_ARITHMETIC_OP2(op)									\
template															\
<																	\
	typename TDomain,												\
	template <typename D_, typename V_> class TLeftHandle,			\
	template <typename D_, typename V_> class TRightHandle,			\
	typename TLeftVal,												\
	typename TRightVal,												\
	class = std::enable_if<std::is_base_of<							\
		Signal_<TDomain,TLeftVal>,									\
		TLeftHandle<TDomain,TLeftVal>>::value>::type,				\
	class = std::enable_if<std::is_base_of<							\
		Signal_<TDomain,TRightVal>,									\
		TRightHandle<TDomain,TLeftVal>>::value>::type				\
>																	\
inline auto operator ## op(const TLeftHandle<TDomain,TLeftVal>& lhs, \
						   const TRightHandle<TDomain,TRightVal>& rhs) \
	-> Signal_<TDomain,TLeftVal>									\
{																	\
	return Signal_<TDomain,TLeftVal>(								\
		std::make_shared<FunctionNode<TDomain,TLeftVal,TLeftVal,TRightVal>>( \
			lhs.GetPtr(), rhs.GetPtr(), [] (TLeftVal a, TRightVal b) { return a op b; }, false)); \
}																	\
																	\
template															\
<																	\
	typename TDomain,												\
	template <typename D_, typename V_> class TLeftHandle,			\
	typename TLeftVal,												\
	typename TRightVal,												\
	class = std::enable_if<std::is_base_of<							\
		Signal_<TDomain,TLeftVal>,									\
		TLeftHandle<TDomain,TLeftVal>>::value>::type,				\
	class = std::enable_if<											\
		std::is_integral<TRightVal>::value>::type					\
>																	\
inline auto operator ## op(const TLeftHandle<TDomain,TLeftVal>& lhs, \
						   const TRightVal& rhs)					\
	-> Signal_<TDomain,TLeftVal>									\
{																	\
	return Signal_<TDomain,TLeftVal>(								\
		std::make_shared<FunctionNode<TDomain,TLeftVal,TLeftVal>>(	\
			lhs.GetPtr(), [=] (TLeftVal a) { return a op rhs; }, false)); \
}																	\
																	\
template															\
<																	\
	typename TDomain,												\
	template <typename D_, typename V_> class TRightHandle,			\
	typename TLeftVal,												\
	typename TRightVal,												\
	class = std::enable_if<std::is_base_of<							\
		Signal_<TDomain,TRightVal>,									\
		TRightHandle<TDomain,TRightVal>>::value>::type,				\
	class = std::enable_if<											\
		std::is_integral<TLeftVal>::value>::type					\
>																	\
inline auto operator ## op(const TLeftVal& lhs,						\
						   const TRightHandle<TDomain,TRightVal>& rhs) \
	-> Signal_<TDomain,TRightVal>									\
{																	\
	return Signal_<TDomain,TRightVal>(								\
		std::make_shared<FunctionNode<TDomain,TRightVal,TRightVal>>( \
			rhs.GetPtr(), [=] (TRightVal a) { return lhs op a; }, false)); \
}

DECLARE_ARITHMETIC_OP2(+);
DECLARE_ARITHMETIC_OP2(-);
DECLARE_ARITHMETIC_OP2(*);
DECLARE_ARITHMETIC_OP2(/);
DECLARE_ARITHMETIC_OP2(%);

#undef DECLARE_ARITHMETIC_OP2

////////////////////////////////////////////////////////////////////////////////////////
/// Comparison operators
////////////////////////////////////////////////////////////////////////////////////////
#define DECLARE_COMP_OP(op)											\
template															\
<																	\
	typename TDomain,												\
	typename TLeftVal,												\
	typename TRightVal												\
>																	\
inline auto operator ## op(const Signal_<TDomain,TLeftVal>& lhs,	\
						   const Signal_<TDomain,TRightVal>& rhs)	\
	-> Signal_<TDomain,bool>										\
{																	\
	return Signal_<TDomain,bool>(									\
		std::make_shared<FunctionNode<TDomain,bool,TLeftVal,TRightVal>>( \
			lhs.GetPtr(), rhs.GetPtr(), [] (TLeftVal a, TRightVal b) { return a op b; }, false)); \
}																	\
																	\
template															\
<																	\
	typename TDomain,												\
	typename TLeftVal,												\
	typename TRightVal												\
>																	\
inline auto operator ## op(const Signal_<TDomain,TLeftVal>& lhs, const TRightVal& rhs) \
	-> Signal_<TDomain,bool>											\
{																	\
	return Signal_<TDomain,bool>(									\
		std::make_shared<FunctionNode<TDomain,bool,TLeftVal>>(		\
			lhs.GetPtr(), [=] (TLeftVal a) { return a op rhs; }, false)); \
}

DECLARE_COMP_OP(==);
DECLARE_COMP_OP(!=);
DECLARE_COMP_OP(<);
DECLARE_COMP_OP(<=);
DECLARE_COMP_OP(>);
DECLARE_COMP_OP(>=);

#undef DECLARE_COMP_OP

////////////////////////////////////////////////////////////////////////////////////////
/// Unary logical operators
////////////////////////////////////////////////////////////////////////////////////////
#define DECLARE_LOGICAL_OP1(op)										\
template															\
<																	\
	typename TDomain,												\
	typename TVal													\
>																	\
inline auto operator ## op(const Signal_<TDomain,TVal>& arg)		\
	-> Signal_<TDomain,bool>										\
{																	\
	return Signal_<TDomain,TVal>(									\
		std::make_shared<FunctionNode<TDomain,bool,TVal>>(			\
			arg.GetPtr(), [] (TVal a) { return op a; }, false));	\
}

DECLARE_LOGICAL_OP1(!);

#undef DECLARE_LOGICAL_OP1

////////////////////////////////////////////////////////////////////////////////////////
/// Binary logical operators
////////////////////////////////////////////////////////////////////////////////////////
#define DECLARE_LOGICAL_OP2(op)										\
template															\
<																	\
	typename TDomain,												\
	typename TLeftVal,												\
	typename TRightVal												\
>																	\
inline auto operator ## op(const Signal_<TDomain,TLeftVal>& lhs,	\
						   const Signal_<TDomain,TRightVal>& rhs)	\
	-> Signal_<TDomain,bool>										\
{																	\
	return Signal_<TDomain,bool>(									\
		std::make_shared<FunctionNode<TDomain,bool,TLeftVal,TRightVal>>( \
			lhs.GetPtr(), rhs.GetPtr(), [] (TLeftVal a, TRightVal b) { return a op b; }, false)); \
}																	\
																	\
template															\
<																	\
	typename TDomain,												\
	typename TLeftVal,												\
	typename TRightVal												\
>																	\
inline auto operator ## op(const Signal_<TDomain,TLeftVal>& lhs, const TRightVal& rhs) \
	-> Signal_<TDomain,bool>										\
{																	\
	return Signal_<TDomain,bool>(									\
		std::make_shared<FunctionNode<TDomain,bool,TLeftVal>>(		\
			lhs.GetPtr(), [=] (TLeftVal a) { return a op rhs; }, false)); \
}

DECLARE_LOGICAL_OP2(&&);
DECLARE_LOGICAL_OP2(||);

#undef DECLARE_LOGICAL_OP2

////////////////////////////////////////////////////////////////////////////////////////
/// InputPack - Wraps several nodes in a tuple. Create with comma operator.
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename ... TValues
>
struct InputPack
{
	std::tuple<Signal_<TDomain, TValues> ...> Data;

	template <typename TFirstValue, typename TSecondValue>
	InputPack(const Signal_<TDomain,TFirstValue>& first, const Signal_<TDomain,TSecondValue>& second) :
		Data(std::make_tuple(first, second))
	{
	}

	template <typename ... TCurValues, typename TAppendValue>
	InputPack(const InputPack<TDomain, TCurValues ...>& curArgs, const Signal_<TDomain,TAppendValue>& newArg) :
		Data(std::tuple_cat(curArgs.Data, std::make_tuple(newArg)))
	{
	}
};

////////////////////////////////////////////////////////////////////////////////////////
/// Comma operator overload to create input pack from 2 nodes.
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename TLeftVal,
	typename TRightVal
>
inline auto operator,(const Signal_<TDomain,TLeftVal>& a, const Signal_<TDomain,TRightVal>& b)
	-> InputPack<TDomain,TLeftVal, TRightVal>
{
	return InputPack<TDomain, TLeftVal, TRightVal>(a, b);
}

////////////////////////////////////////////////////////////////////////////////////////
/// Comma operator overload to append node to existing input pack.
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename ... TCurValues,
	typename TAppendValue
>
inline auto operator,(const InputPack<TDomain, TCurValues ...>& cur, const Signal_<TDomain,TAppendValue>& append)
	-> InputPack<TDomain,TCurValues ... , TAppendValue>
{
	return InputPack<TDomain, TCurValues ... , TAppendValue>(cur, append);
}

////////////////////////////////////////////////////////////////////////////////////////
/// operator>>= overload to connect inputs to a function and return the resulting node.
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename TFunc,
	typename ... TValues
>
struct ApplyHelper_
{
	static inline auto MakeSignal(const TFunc& func, const Signal_<TDomain,TValues>& ... args)
		-> decltype(TDomain::MakeSignal(func, args ...))
	{
		return TDomain::MakeSignal(func, args ...);
	}
};

// Single input
template
<
	typename TDomain,
	typename TFunc,
	typename TValue
>
inline auto operator>>=(const Signal_<TDomain,TValue>& inputNode, TFunc func)
	-> decltype(TDomain::MakeSignal(func, inputNode))
{
	return TDomain::MakeSignal(func, inputNode);
}

// Multiple inputs
template
<
	typename TDomain,
	typename TFunc,
	typename ... TValues
>
inline auto operator>>=(InputPack<TDomain,TValues ...>& inputPack, TFunc func)
	-> decltype(apply(ApplyHelper_<TDomain, TFunc, TValues ...>::MakeSignal, std::tuple_cat(std::make_tuple(func), inputPack.Data)))
{
	return apply(ApplyHelper_<TDomain, TFunc, TValues ...>::MakeSignal, std::tuple_cat(std::make_tuple(func), inputPack.Data));
}

////////////////////////////////////////////////////////////////////////////////////////
/// Flatten
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename TInnerValue
>
inline auto Flatten(const Signal_<TDomain,Signal_<TDomain,TInnerValue>>& node)
	-> Signal_<TDomain,TInnerValue>
{
	return Signal_<TDomain,TInnerValue>(
		std::make_shared<FlattenNode<TDomain, Signal_<TDomain,TInnerValue>, TInnerValue>>(
			node.GetPtr(), node().GetPtr(), false));
}

// ---
}