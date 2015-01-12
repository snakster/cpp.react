
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_SIGNAL_H_INCLUDED
#define REACT_SIGNAL_H_INCLUDED

#pragma once

#if _MSC_VER && !__INTEL_COMPILER
    #pragma warning(disable : 4503)
#endif

#include "react/detail/Defs.h"

#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#include "react/Observer.h"
#include "react/TypeTraits.h"
#include "react/detail/SignalBase.h"

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward declarations
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename S>
class Signal;

template <typename D, typename S>
class VarSignal;

template <typename D, typename S, typename TOp>
class TempSignal;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SignalPack - Wraps several nodes in a tuple. Create with comma operator.
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename ... TValues
>
class SignalPack
{
public:
    SignalPack(const Signal<D,TValues>&  ... deps) :
        Data( std::tie(deps ...) )
    {}

    template <typename ... TCurValues, typename TAppendValue>
    SignalPack(const SignalPack<D, TCurValues ...>& curArgs, const Signal<D,TAppendValue>& newArg) :
        Data( std::tuple_cat(curArgs.Data, std::tie(newArg)) )
    {}

    std::tuple<const Signal<D,TValues>& ...> Data;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// With - Utility function to create a SignalPack
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename ... TValues
>
auto With(const Signal<D,TValues>&  ... deps)
    -> SignalPack<D,TValues ...>
{
    return SignalPack<D,TValues...>(deps ...);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MakeVar
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename V,
    typename S = typename std::decay<V>::type,
    class = typename std::enable_if<
        ! IsSignal<S>::value>::type,
    class = typename std::enable_if<
        ! IsEvent<S>::value>::type
>
auto MakeVar(V&& value)
    -> VarSignal<D,S>
{
    return VarSignal<D,S>(
        std::make_shared<REACT_IMPL::VarNode<D,S>>(
            std::forward<V>(value)));
}

template
<
    typename D,
    typename S
>
auto MakeVar(std::reference_wrapper<S> value)
    -> VarSignal<D,S&>
{
    return VarSignal<D,S&>(
        std::make_shared<REACT_IMPL::VarNode<D,std::reference_wrapper<S>>>(value));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MakeVar (higher order reactives)
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename V,
    typename S = typename std::decay<V>::type,
    typename TInner = typename S::ValueT,
    class = typename std::enable_if<
        IsSignal<S>::value>::type
>
auto MakeVar(V&& value)
    -> VarSignal<D,Signal<D,TInner>>
{
    return VarSignal<D,Signal<D,TInner>>(
        std::make_shared<REACT_IMPL::VarNode<D,Signal<D,TInner>>>(
            std::forward<V>(value)));
}

template
<
    typename D,
    typename V,
    typename S = typename std::decay<V>::type,
    typename TInner = typename S::ValueT,
    class = typename std::enable_if<
        IsEvent<S>::value>::type
>
auto MakeVar(V&& value)
    -> VarSignal<D,Events<D,TInner>>
{
    return VarSignal<D,Events<D,TInner>>(
        std::make_shared<REACT_IMPL::VarNode<D,Events<D,TInner>>>(
            std::forward<V>(value)));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MakeSignal
///////////////////////////////////////////////////////////////////////////////////////////////////
// Single arg
template
<
    typename D,
    typename TValue,
    typename FIn,
    typename F = typename std::decay<FIn>::type,
    typename S = typename std::result_of<F(TValue)>::type,
    typename TOp = REACT_IMPL::FunctionOp<S,F, REACT_IMPL::SignalNodePtrT<D,TValue>>
>
auto MakeSignal(const Signal<D,TValue>& arg, FIn&& func)
    -> TempSignal<D,S,TOp>
{
    return TempSignal<D,S,TOp>(
        std::make_shared<REACT_IMPL::SignalOpNode<D,S,TOp>>(
            std::forward<FIn>(func), GetNodePtr(arg)));
}

// Multiple args
template
<
    typename D,
    typename ... TValues,
    typename FIn,
    typename F = typename std::decay<FIn>::type,
    typename S = typename std::result_of<F(TValues...)>::type,
    typename TOp = REACT_IMPL::FunctionOp<S,F, REACT_IMPL::SignalNodePtrT<D,TValues> ...>
>
auto MakeSignal(const SignalPack<D,TValues...>& argPack, FIn&& func)
    -> TempSignal<D,S,TOp>
{
    using REACT_IMPL::SignalOpNode;

    struct NodeBuilder_
    {
        NodeBuilder_(FIn&& func) :
            MyFunc( std::forward<FIn>(func) )
        {}

        auto operator()(const Signal<D,TValues>& ... args)
            -> TempSignal<D,S,TOp>
        {
            return TempSignal<D,S,TOp>(
                std::make_shared<SignalOpNode<D,S,TOp>>(
                    std::forward<FIn>(MyFunc), GetNodePtr(args) ...));
        }

        FIn     MyFunc;
    };

    return REACT_IMPL::apply(
        NodeBuilder_( std::forward<FIn>(func) ),
        argPack.Data);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Unary operators
///////////////////////////////////////////////////////////////////////////////////////////////////
#define REACT_DECLARE_OP(op,name)                                                   \
template <typename T>                                                               \
struct name ## OpFunctor                                                            \
{                                                                                   \
    T operator()(const T& v) const { return op v; }                                 \
};                                                                                  \
                                                                                    \
template                                                                            \
<                                                                                   \
    typename TSignal,                                                               \
    typename D = typename TSignal::DomainT,                                         \
    typename TVal = typename TSignal::ValueT,                                       \
    class = typename std::enable_if<                                                \
        IsSignal<TSignal>::value>::type,                                            \
    typename F = name ## OpFunctor<TVal>,                                           \
    typename S = typename std::result_of<F(TVal)>::type,                            \
    typename TOp = REACT_IMPL::FunctionOp<S,F,REACT_IMPL::SignalNodePtrT<D,TVal>>   \
>                                                                                   \
auto operator op(const TSignal& arg)                                                \
    -> TempSignal<D,S,TOp>                                                          \
{                                                                                   \
    return TempSignal<D,S,TOp>(                                                     \
        std::make_shared<REACT_IMPL::SignalOpNode<D,S,TOp>>(                        \
            F(), GetNodePtr(arg)));                                                 \
}                                                                                   \
                                                                                    \
template                                                                            \
<                                                                                   \
    typename D,                                                                     \
    typename TVal,                                                                  \
    typename TOpIn,                                                                 \
    typename F = name ## OpFunctor<TVal>,                                           \
    typename S = typename std::result_of<F(TVal)>::type,                            \
    typename TOp = REACT_IMPL::FunctionOp<S,F,TOpIn>                                \
>                                                                                   \
auto operator op(TempSignal<D,TVal,TOpIn>&& arg)                                    \
    -> TempSignal<D,S,TOp>                                                          \
{                                                                                   \
    return TempSignal<D,S,TOp>(                                                     \
        std::make_shared<REACT_IMPL::SignalOpNode<D,S,TOp>>(                        \
            F(), arg.StealOp()));                                                   \
}

REACT_DECLARE_OP(+, UnaryPlus)
REACT_DECLARE_OP(-, UnaryMinus)
REACT_DECLARE_OP(!, LogicalNegation)
REACT_DECLARE_OP(~, BitwiseComplement)
REACT_DECLARE_OP(++, Increment)
REACT_DECLARE_OP(--, Decrement)

#undef REACT_DECLARE_OP                                                                      

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Binary operators
///////////////////////////////////////////////////////////////////////////////////////////////////
#define REACT_DECLARE_OP(op,name)                                                   \
template <typename L, typename R>                                                   \
struct name ## OpFunctor                                                            \
{                                                                                   \
    auto operator()(const L& lhs, const R& rhs) const                               \
        -> decltype(std::declval<L>() op std::declval<R>())                         \
    {                                                                               \
        return lhs op rhs;                                                          \
    }                                                                               \
};                                                                                  \
                                                                                    \
template <typename L, typename R>                                                   \
struct name ## OpRFunctor                                                           \
{                                                                                   \
    name ## OpRFunctor(name ## OpRFunctor&& other) :                                \
        LeftVal( std::move(other.LeftVal) )                                         \
    {}                                                                              \
                                                                                    \
    template <typename T>                                                           \
    name ## OpRFunctor(T&& val) :                                                   \
        LeftVal( std::forward<T>(val) )                                             \
    {}                                                                              \
                                                                                    \
    name ## OpRFunctor(const name ## OpRFunctor& other) = delete;                   \
                                                                                    \
    auto operator()(const R& rhs) const                                             \
        -> decltype(std::declval<L>() op std::declval<R>())                         \
    {                                                                               \
        return LeftVal op rhs;                                                      \
    }                                                                               \
                                                                                    \
    L LeftVal;                                                                      \
};                                                                                  \
                                                                                    \
template <typename L, typename R>                                                   \
struct name ## OpLFunctor                                                           \
{                                                                                   \
    name ## OpLFunctor(name ## OpLFunctor&& other) :                                \
        RightVal( std::move(other.RightVal) )                                       \
    {}                                                                              \
                                                                                    \
    template <typename T>                                                           \
    name ## OpLFunctor(T&& val) :                                                   \
        RightVal( std::forward<T>(val) )                                            \
    {}                                                                              \
                                                                                    \
    name ## OpLFunctor(const name ## OpLFunctor& other) = delete;                   \
                                                                                    \
    auto operator()(const L& lhs) const                                             \
        -> decltype(std::declval<L>() op std::declval<R>())                         \
    {                                                                               \
        return lhs op RightVal;                                                     \
    }                                                                               \
                                                                                    \
    R RightVal;                                                                     \
};                                                                                  \
                                                                                    \
template                                                                            \
<                                                                                   \
    typename TLeftSignal,                                                           \
    typename TRightSignal,                                                          \
    typename D = typename TLeftSignal::DomainT,                                     \
    typename TLeftVal = typename TLeftSignal::ValueT,                               \
    typename TRightVal = typename TRightSignal::ValueT,                             \
    class = typename std::enable_if<                                                \
        IsSignal<TLeftSignal>::value>::type,                                        \
    class = typename std::enable_if<                                                \
        IsSignal<TRightSignal>::value>::type,                                       \
    typename F = name ## OpFunctor<TLeftVal,TRightVal>,                             \
    typename S = typename std::result_of<F(TLeftVal,TRightVal)>::type,              \
    typename TOp = REACT_IMPL::FunctionOp<S,F,                                      \
        REACT_IMPL::SignalNodePtrT<D,TLeftVal>,                                     \
        REACT_IMPL::SignalNodePtrT<D,TRightVal>>                                    \
>                                                                                   \
auto operator op(const TLeftSignal& lhs, const TRightSignal& rhs)                   \
    -> TempSignal<D,S,TOp>                                                          \
{                                                                                   \
    return TempSignal<D,S,TOp>(                                                     \
        std::make_shared<REACT_IMPL::SignalOpNode<D,S,TOp>>(                        \
            F(), GetNodePtr(lhs), GetNodePtr(rhs)));                                \
}                                                                                   \
                                                                                    \
template                                                                            \
<                                                                                   \
    typename TLeftSignal,                                                           \
    typename TRightValIn,                                                           \
    typename D = typename TLeftSignal::DomainT,                                     \
    typename TLeftVal = typename TLeftSignal::ValueT,                               \
    typename TRightVal = typename std::decay<TRightValIn>::type,                    \
    class = typename std::enable_if<                                                \
        IsSignal<TLeftSignal>::value>::type,                                        \
    class = typename std::enable_if<                                                \
        ! IsSignal<TRightVal>::value>::type,                                        \
    typename F = name ## OpLFunctor<TLeftVal,TRightVal>,                            \
    typename S = typename std::result_of<F(TLeftVal)>::type,                        \
    typename TOp = REACT_IMPL::FunctionOp<S,F,                                      \
        REACT_IMPL::SignalNodePtrT<D,TLeftVal>>                                     \
>                                                                                   \
auto operator op(const TLeftSignal& lhs, TRightValIn&& rhs)                         \
    -> TempSignal<D,S,TOp>                                                          \
{                                                                                   \
    return TempSignal<D,S,TOp>(                                                     \
        std::make_shared<REACT_IMPL::SignalOpNode<D,S,TOp>>(                        \
            F( std::forward<TRightValIn>(rhs) ), GetNodePtr(lhs)));                 \
}                                                                                   \
                                                                                    \
template                                                                            \
<                                                                                   \
    typename TLeftValIn,                                                            \
    typename TRightSignal,                                                          \
    typename D = typename TRightSignal::DomainT,                                    \
    typename TLeftVal = typename std::decay<TLeftValIn>::type,                      \
    typename TRightVal = typename TRightSignal::ValueT,                             \
    class = typename std::enable_if<                                                \
        ! IsSignal<TLeftVal>::value>::type,                                         \
    class = typename std::enable_if<                                                \
        IsSignal<TRightSignal>::value>::type,                                       \
    typename F = name ## OpRFunctor<TLeftVal,TRightVal>,                            \
    typename S = typename std::result_of<F(TRightVal)>::type,                       \
    typename TOp = REACT_IMPL::FunctionOp<S,F,                                      \
        REACT_IMPL::SignalNodePtrT<D,TRightVal>>                                    \
>                                                                                   \
auto operator op(TLeftValIn&& lhs, const TRightSignal& rhs)                         \
    -> TempSignal<D,S,TOp>                                                          \
{                                                                                   \
    return TempSignal<D,S,TOp>(                                                     \
        std::make_shared<REACT_IMPL::SignalOpNode<D,S,TOp>>(                        \
            F( std::forward<TLeftValIn>(lhs) ), GetNodePtr(rhs)));                  \
}                                                                                   \
template                                                                            \
<                                                                                   \
    typename D,                                                                     \
    typename TLeftVal,                                                              \
    typename TLeftOp,                                                               \
    typename TRightVal,                                                             \
    typename TRightOp,                                                              \
    typename F = name ## OpFunctor<TLeftVal,TRightVal>,                             \
    typename S = typename std::result_of<F(TLeftVal,TRightVal)>::type,              \
    typename TOp = REACT_IMPL::FunctionOp<S,F,TLeftOp,TRightOp>                     \
>                                                                                   \
auto operator op(TempSignal<D,TLeftVal,TLeftOp>&& lhs,                              \
                 TempSignal<D,TRightVal,TRightOp>&& rhs)                            \
    -> TempSignal<D,S,TOp>                                                          \
{                                                                                   \
    return TempSignal<D,S,TOp>(                                                     \
        std::make_shared<REACT_IMPL::SignalOpNode<D,S,TOp>>(                        \
            F(), lhs.StealOp(), rhs.StealOp()));                                    \
}                                                                                   \
                                                                                    \
template                                                                            \
<                                                                                   \
    typename D,                                                                     \
    typename TLeftVal,                                                              \
    typename TLeftOp,                                                               \
    typename TRightSignal,                                                          \
    typename TRightVal = typename TRightSignal::ValueT,                             \
    class = typename std::enable_if<                                                \
        IsSignal<TRightSignal>::value>::type,                                       \
    typename F = name ## OpFunctor<TLeftVal,TRightVal>,                             \
    typename S = typename std::result_of<F(TLeftVal,TRightVal)>::type,              \
    typename TOp = REACT_IMPL::FunctionOp<S,F,                                      \
        TLeftOp,                                                                    \
        REACT_IMPL::SignalNodePtrT<D,TRightVal>>                                    \
>                                                                                   \
auto operator op(TempSignal<D,TLeftVal,TLeftOp>&& lhs,                              \
                 const TRightSignal& rhs)                                           \
    -> TempSignal<D,S,TOp>                                                          \
{                                                                                   \
    return TempSignal<D,S,TOp>(                                                     \
        std::make_shared<REACT_IMPL::SignalOpNode<D,S,TOp>>(                        \
            F(), lhs.StealOp(), GetNodePtr(rhs)));                                  \
}                                                                                   \
                                                                                    \
template                                                                            \
<                                                                                   \
    typename TLeftSignal,                                                           \
    typename D,                                                                     \
    typename TRightVal,                                                             \
    typename TRightOp,                                                              \
    typename TLeftVal = typename TLeftSignal::ValueT,                               \
    class = typename std::enable_if<                                                \
        IsSignal<TLeftSignal>::value>::type,                                        \
    typename F = name ## OpFunctor<TLeftVal,TRightVal>,                             \
    typename S = typename std::result_of<F(TLeftVal,TRightVal)>::type,              \
    typename TOp = REACT_IMPL::FunctionOp<S,F,                                      \
        REACT_IMPL::SignalNodePtrT<D,TLeftVal>,                                     \
        TRightOp>                                                                   \
>                                                                                   \
auto operator op(const TLeftSignal& lhs, TempSignal<D,TRightVal,TRightOp>&& rhs)    \
    -> TempSignal<D,S,TOp>                                                          \
{                                                                                   \
    return TempSignal<D,S,TOp>(                                                     \
        std::make_shared<REACT_IMPL::SignalOpNode<D,S,TOp>>(                        \
            F(), GetNodePtr(lhs), rhs.StealOp()));                                  \
}                                                                                   \
                                                                                    \
template                                                                            \
<                                                                                   \
    typename D,                                                                     \
    typename TLeftVal,                                                              \
    typename TLeftOp,                                                               \
    typename TRightValIn,                                                           \
    typename TRightVal = typename std::decay<TRightValIn>::type,                    \
    class = typename std::enable_if<                                                \
        ! IsSignal<TRightVal>::value>::type,                                        \
    typename F = name ## OpLFunctor<TLeftVal,TRightVal>,                            \
    typename S = typename std::result_of<F(TLeftVal)>::type,                        \
    typename TOp = REACT_IMPL::FunctionOp<S,F,TLeftOp>                              \
>                                                                                   \
auto operator op(TempSignal<D,TLeftVal,TLeftOp>&& lhs, TRightValIn&& rhs)           \
    -> TempSignal<D,S,TOp>                                                          \
{                                                                                   \
    return TempSignal<D,S,TOp>(                                                     \
        std::make_shared<REACT_IMPL::SignalOpNode<D,S,TOp>>(                        \
            F( std::forward<TRightValIn>(rhs) ), lhs.StealOp()));                   \
}                                                                                   \
                                                                                    \
template                                                                            \
<                                                                                   \
    typename TLeftValIn,                                                            \
    typename D,                                                                     \
    typename TRightVal,                                                             \
    typename TRightOp,                                                              \
    typename TLeftVal = typename std::decay<TLeftValIn>::type,                      \
    class = typename std::enable_if<                                                \
        ! IsSignal<TLeftVal>::value>::type,                                         \
    typename F = name ## OpRFunctor<TLeftVal,TRightVal>,                            \
    typename S = typename std::result_of<F(TRightVal)>::type,                       \
    typename TOp = REACT_IMPL::FunctionOp<S,F,TRightOp>                             \
>                                                                                   \
auto operator op(TLeftValIn&& lhs, TempSignal<D,TRightVal,TRightOp>&& rhs)          \
    -> TempSignal<D,S,TOp>                                                          \
{                                                                                   \
    return TempSignal<D,S,TOp>(                                                     \
        std::make_shared<REACT_IMPL::SignalOpNode<D,S,TOp>>(                        \
            F( std::forward<TLeftValIn>(lhs) ), rhs.StealOp()));                    \
}                                                                                   

REACT_DECLARE_OP(+, Addition)
REACT_DECLARE_OP(-, Subtraction)
REACT_DECLARE_OP(*, Multiplication)
REACT_DECLARE_OP(/, Division)
REACT_DECLARE_OP(%, Modulo)

REACT_DECLARE_OP(==, Equal)
REACT_DECLARE_OP(!=, NotEqual)
REACT_DECLARE_OP(<, Less)
REACT_DECLARE_OP(<=, LessEqual)
REACT_DECLARE_OP(>, Greater)
REACT_DECLARE_OP(>=, GreaterEqual)

REACT_DECLARE_OP(&&, LogicalAnd)
REACT_DECLARE_OP(||, LogicalOr)

REACT_DECLARE_OP(&, BitwiseAnd)
REACT_DECLARE_OP(|, BitwiseOr)
REACT_DECLARE_OP(^, BitwiseXor)
//REACT_DECLARE_OP(<<, BitwiseLeftShift); // MSVC: Internal compiler error
//REACT_DECLARE_OP(>>, BitwiseRightShift);

#undef REACT_DECLARE_OP 

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Comma operator overload to create signal pack from 2 signals.
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename TLeftVal,
    typename TRightVal
>
auto operator,(const Signal<D,TLeftVal>& a, const Signal<D,TRightVal>& b)
    -> SignalPack<D,TLeftVal, TRightVal>
{
    return SignalPack<D, TLeftVal, TRightVal>(a, b);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Comma operator overload to append node to existing signal pack.
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename ... TCurValues,
    typename TAppendValue
>
auto operator,(const SignalPack<D, TCurValues ...>& cur, const Signal<D,TAppendValue>& append)
    -> SignalPack<D,TCurValues ... , TAppendValue>
{
    return SignalPack<D, TCurValues ... , TAppendValue>(cur, append);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// operator->* overload to connect signals to a function and return the resulting signal.
///////////////////////////////////////////////////////////////////////////////////////////////////
// Single arg
template
<
    typename D,
    typename F,
    template <typename D_, typename V_> class TSignal,
    typename TValue,
    class = typename std::enable_if<
        IsSignal<TSignal<D,TValue>>::value>::type
>
auto operator->*(const TSignal<D,TValue>& arg, F&& func)
    -> Signal<D, typename std::result_of<F(TValue)>::type>
{
    return REACT::MakeSignal(arg, std::forward<F>(func));
}

// Multiple args
template
<
    typename D,
    typename F,
    typename ... TValues
>
auto operator->*(const SignalPack<D,TValues ...>& argPack, F&& func)
    -> Signal<D, typename std::result_of<F(TValues ...)>::type>
{
    return REACT::MakeSignal(argPack, std::forward<F>(func));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Flatten
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename TInnerValue
>
auto Flatten(const Signal<D,Signal<D,TInnerValue>>& outer)
    -> Signal<D,TInnerValue>
{
    return Signal<D,TInnerValue>(
        std::make_shared<REACT_IMPL::FlattenNode<D, Signal<D,TInnerValue>, TInnerValue>>(
            GetNodePtr(outer), GetNodePtr(outer.Value())));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Signal
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S
>
class Signal : public REACT_IMPL::SignalBase<D,S>
{
private:
    using NodeT     = REACT_IMPL::SignalNode<D,S>;
    using NodePtrT  = std::shared_ptr<NodeT>;

public:
    using ValueT = S;

    // Default ctor
    Signal() = default;

    // Copy ctor
    Signal(const Signal&) = default;

    // Move ctor
    Signal(Signal&& other) :
        Signal::SignalBase( std::move(other) )
    {}

    // Node ctor
    explicit Signal(NodePtrT&& nodePtr) :
        Signal::SignalBase( std::move(nodePtr) )
    {}

    // Copy assignment
    Signal& operator=(const Signal&) = default;

    // Move assignment
    Signal& operator=(Signal&& other)
    {
        Signal::SignalBase::operator=( std::move(other) );
        return *this;
    }

    const S& Value() const      { return Signal::SignalBase::getValue(); }
    const S& operator()() const { return Signal::SignalBase::getValue(); }

    bool Equals(const Signal& other) const
    {
        return Signal::SignalBase::Equals(other);
    }

    bool IsValid() const
    {
        return Signal::SignalBase::IsValid();
    }

    void SetWeightHint(WeightHint weight)
    {
        Signal::SignalBase::SetWeightHint(weight);
    }

    S Flatten() const
    {
        static_assert(IsSignal<S>::value || IsEvent<S>::value,
            "Flatten requires a Signal or Events value type.");
        return REACT::Flatten(*this);
    }
};

// Specialize for references
template
<
    typename D,
    typename S
>
class Signal<D,S&> : public REACT_IMPL::SignalBase<D,std::reference_wrapper<S>>
{
private:
    using NodeT     = REACT_IMPL::SignalNode<D,std::reference_wrapper<S>>;
    using NodePtrT  = std::shared_ptr<NodeT>;

public:
    using ValueT = S;

    // Default ctor
    Signal() = default;

    // Copy ctor
    Signal(const Signal&) = default;

    // Move ctor
    Signal(Signal&& other) :
        Signal::SignalBase( std::move(other) )
    {}

    // Node ctor
    explicit Signal(NodePtrT&& nodePtr) :
        Signal::SignalBase( std::move(nodePtr) )
    {}

    // Copy assignment
    Signal& operator=(const Signal&) = default;

    // Move assignment
    Signal& operator=(Signal&& other)
    {
        Signal::SignalBase::operator=( std::move(other) );
        return *this;
    }

    const S& Value() const      { return Signal::SignalBase::getValue(); }
    const S& operator()() const { return Signal::SignalBase::getValue(); }

    bool Equals(const Signal& other) const
    {
        return Signal::SignalBase::Equals(other);
    }

    bool IsValid() const
    {
        return Signal::SignalBase::IsValid();
    }

    void SetWeightHint(WeightHint weight)
    {
        Signal::SignalBase::SetWeightHint(weight);
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// VarSignal
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S
>
class VarSignal : public Signal<D,S>
{
private:
    using NodeT     = REACT_IMPL::VarNode<D,S>;
    using NodePtrT  = std::shared_ptr<NodeT>;

public:
    // Default ctor
    VarSignal() = default;

    // Copy ctor
    VarSignal(const VarSignal&) = default;

    // Move ctor
    VarSignal(VarSignal&& other) :
        VarSignal::Signal( std::move(other) )
    {}

    // Node ctor
    explicit VarSignal(NodePtrT&& nodePtr) :
        VarSignal::Signal( std::move(nodePtr) )
    {}

    // Copy assignment
    VarSignal& operator=(const VarSignal&) = default;

    // Move assignment
    VarSignal& operator=(VarSignal&& other)
    {
        VarSignal::SignalBase::operator=( std::move(other) );
        return *this;
    }

    void Set(const S& newValue) const
    {
        VarSignal::SignalBase::setValue(newValue);
    }

    void Set(S&& newValue) const
    {
        VarSignal::SignalBase::setValue(std::move(newValue));
    }

    const VarSignal& operator<<=(const S& newValue) const
    {
        VarSignal::SignalBase::setValue(newValue);
        return *this;
    }

    const VarSignal& operator<<=(S&& newValue) const
    {
        VarSignal::SignalBase::setValue(std::move(newValue));
        return *this;
    }

    template <typename F>
    void Modify(const F& func) const
    {
        VarSignal::SignalBase::modifyValue(func);
    }
};

// Specialize for references
template
<
    typename D,
    typename S
>
class VarSignal<D,S&> : public Signal<D,std::reference_wrapper<S>>
{
private:
    using NodeT     = REACT_IMPL::VarNode<D,std::reference_wrapper<S>>;
    using NodePtrT  = std::shared_ptr<NodeT>;

public:
    using ValueT = S;

    // Default ctor
    VarSignal() = default;

    // Copy ctor
    VarSignal(const VarSignal&) = default;

    // Move ctor
    VarSignal(VarSignal&& other) :
        VarSignal::Signal( std::move(other) )
    {}

    // Node ctor
    explicit VarSignal(NodePtrT&& nodePtr) :
        VarSignal::Signal( std::move(nodePtr) )
    {}

    // Copy assignment
    VarSignal& operator=(const VarSignal&) = default;

    // Move assignment
    VarSignal& operator=(VarSignal&& other)
    {
        VarSignal::Signal::operator=( std::move(other) );
        return *this;
    }

    void Set(std::reference_wrapper<S> newValue) const
    {
        VarSignal::SignalBase::setValue(newValue);
    }

    const VarSignal& operator<<=(std::reference_wrapper<S> newValue) const
    {
        VarSignal::SignalBase::setValue(newValue);
        return *this;
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// TempSignal
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S,
    typename TOp
>
class TempSignal : public Signal<D,S>
{
private:
    using NodeT     = REACT_IMPL::SignalOpNode<D,S,TOp>;
    using NodePtrT  = std::shared_ptr<NodeT>;

public:
    // Default ctor
    TempSignal() = default;

    // Copy ctor
    TempSignal(const TempSignal&) = default;

    // Move ctor
    TempSignal(TempSignal&& other) :
        TempSignal::Signal( std::move(other) )
    {}

    // Node ctor
    explicit TempSignal(NodePtrT&& ptr) :
        TempSignal::Signal( std::move(ptr) )
    {}

    // Copy assignment
    TempSignal& operator=(const TempSignal&) = default;

    // Move assignemnt
    TempSignal& operator=(TempSignal&& other)
    {
        TempSignal::Signal::operator=( std::move(other) );
        return *this;
    }

    TOp StealOp()
    {
        return std::move(reinterpret_cast<NodeT*>(this->ptr_.get())->StealOp());
    }
};

/******************************************/ REACT_END /******************************************/

/***************************************/ REACT_IMPL_BEGIN /**************************************/

template <typename D, typename L, typename R>
bool Equals(const Signal<D,L>& lhs, const Signal<D,R>& rhs)
{
    return lhs.Equals(rhs);
}

/****************************************/ REACT_IMPL_END /***************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Flatten macros
///////////////////////////////////////////////////////////////////////////////////////////////////
// Note: Using static_cast rather than -> return type, because when using lambda for inline
// class initialization, decltype did not recognize the parameter r
// Note2: MSVC doesn't like typename in the lambda
#if _MSC_VER && !__INTEL_COMPILER
    #define REACT_MSVC_NO_TYPENAME
#else
    #define REACT_MSVC_NO_TYPENAME typename
#endif

#define REACTIVE_REF(obj, name)                                                             \
    Flatten(                                                                                \
        MakeSignal(                                                                         \
            obj,                                                                            \
            [] (const REACT_MSVC_NO_TYPENAME                                                \
                REACT_IMPL::Identity<decltype(obj)>::Type::ValueT& r)                       \
            {                                                                               \
                using T = decltype(r.name);                                                 \
                using S = REACT_MSVC_NO_TYPENAME REACT::DecayInput<T>::Type;                \
                return static_cast<S>(r.name);                                              \
            }))

#define REACTIVE_PTR(obj, name)                                                             \
    Flatten(                                                                                \
        MakeSignal(                                                                         \
            obj,                                                                            \
            [] (REACT_MSVC_NO_TYPENAME                                                      \
                REACT_IMPL::Identity<decltype(obj)>::Type::ValueT r)                        \
            {                                                                               \
                assert(r != nullptr);                                                       \
                using T = decltype(r->name);                                                \
                using S = REACT_MSVC_NO_TYPENAME REACT::DecayInput<T>::Type;                \
                return static_cast<S>(r->name);                                             \
            }))

#endif // REACT_SIGNAL_H_INCLUDED