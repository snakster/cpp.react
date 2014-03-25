
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

#include "ReactiveBase.h"
#include "ReactiveDomain.h"

#include "react/common/Util.h"
#include "react/graph/SignalNodes.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

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

/****************************************/ REACT_IMPL_END /***************************************/

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Signal
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S
>
class Signal : public Reactive<REACT_IMPL::SignalNode<D,S>>
{
protected:
    using NodeT = REACT_IMPL::SignalNode<D,S>;

public:
    using ValueT = S;

    Signal() :
        Reactive()
    {
    }
    
    explicit Signal(const std::shared_ptr<NodeT>& ptr) :
        Reactive(ptr)
    {
    }

    explicit Signal(std::shared_ptr<NodeT>&& ptr) :
        Reactive(std::move(ptr))
    {
    }

    const S& Value() const
    {
        return ptr_->ValueRef();
    }

    const S& operator()(void) const
    {
        return Value();
    }

    template <typename F>
    Observer<D> Observe(F&& f)
    {
        return react::Observe(*this, std::forward<F>(f));
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
protected:
    using NodeT = REACT_IMPL::VarNode<D,S>;

public:    
    VarSignal() :
        Signal()
    {
    }
    
    explicit VarSignal(const std::shared_ptr<NodeT>& ptr) :
        Signal(ptr)
    {
    }

    explicit VarSignal(std::shared_ptr<NodeT>&& ptr) :
        Signal(std::move(ptr))
    {
    }

    template <typename V>
    void Set(V&& newValue) const
    {
        D::AddInput(*std::static_pointer_cast<NodeT>(ptr_), std::forward<V>(newValue));
    }

    template <typename V>
    const VarSignal& operator<<=(V&& newValue) const
    {
        Set(std::forward<V>(newValue));
        return *this;
    }
};

/******************************************/ REACT_END /******************************************/

/***************************************/ REACT_IMPL_BEGIN /**************************************/

template <typename D, typename L, typename R>
bool Equals(const Signal<D,L>& lhs, const Signal<D,R>& rhs)
{
    return lhs.Equals(rhs);
}

template <typename D, typename L, typename R>
bool Equals(const VarSignal<D,L>& lhs, const VarSignal<D,R>& rhs)
{
    return lhs.Equals(rhs);
}

/****************************************/ REACT_IMPL_END /***************************************/

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MakeVar
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename V,
    typename S = std::decay<V>::type,
    class = std::enable_if<
        ! IsReactive<D,S>::value>::type
>
auto MakeVar(V&& value)
    -> VarSignal<D,S>
{
    return VarSignal<D,S>(
        std::make_shared<REACT_IMPL::VarNode<D,S>>(
            std::forward<V>(value), false));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MakeVar (higher order reactives)
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename V,
    typename S = std::decay<V>::type,
    typename TInner = S::ValueT,
    class = std::enable_if<
        IsSignal<D,S>::value>::type
>
auto MakeVar(V&& value)
    -> VarSignal<D,Signal<D,TInner>>
{
    return VarSignal<D,Signal<D,TInner>>(
        std::make_shared<REACT_IMPL::VarNode<D,Signal<D,TInner>>>(
            std::forward<V>(value), false));
}

template
<
    typename D,
    typename V,
    typename S = std::decay<V>::type,
    typename TInner = S::ValueT,
    class = std::enable_if<
        IsEvent<D,S>::value>::type
>
auto MakeVar(V&& value)
    -> VarSignal<D,Events<D,TInner>>
{
    return VarSignal<D,Events<D,TInner>>(
        std::make_shared<REACT_IMPL::VarNode<D,Events<D,TInner>>>(
            std::forward<V>(value), false));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MakeVal
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename V,
    typename S = std::decay<V>::type
>
auto MakeVal(V&& value)
    -> Signal<D,S>
{
    return Signal<D,S>(
        std::make_shared<REACT_IMPL::ValNode<D,S>>(
            std::forward<V>(value), false));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MakeSignal
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename F,
    typename ... TArgs
>
auto MakeSignal(F&& func, const Signal<D,TArgs>& ... args)
    -> Signal<D, typename std::result_of<F(TArgs...)>::type>
{
    static_assert(sizeof...(TArgs) > 0,
        "react::MakeSignal requires at least 1 signal dependency.");

    using S = typename std::result_of<F(TArgs...)>::type;

    return Signal<D,S>(
        std::make_shared<REACT_IMPL::FunctionNode<D, S, TArgs ...>>(
            std::forward<F>(func), args.GetPtr() ..., false));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Unary operators
///////////////////////////////////////////////////////////////////////////////////////////////////
#define DECLARE_OP(op)                                                      \
template                                                                    \
<                                                                           \
    typename D,                                                             \
    template <typename D_, typename V_> class TSignal,                      \
    typename TVal,                                                          \
    class = std::enable_if<                                                 \
        IsSignal<D,TSignal<D,TVal>>::value>::type                           \
>                                                                           \
auto operator ## op(const TSignal<D,TVal>& arg)                             \
    -> Signal<D,decltype(op std::declval<TVal>())>                         \
{                                                                           \
    return Signal<D,decltype(op std::declval<TVal>())>(                    \
        std::make_shared<REACT_IMPL::FunctionNode<D,TVal,TVal>>(            \
            [] (TVal a) { return op a; }, arg.GetPtr(), false));            \
}

DECLARE_OP(+);
DECLARE_OP(-);

DECLARE_OP(!);

#undef DECLARE_OP

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Binary operators
///////////////////////////////////////////////////////////////////////////////////////////////////
#define DECLARE_OP(op)                                                              \
template                                                                            \
<                                                                                   \
    typename D,                                                                     \
    template <typename D_, typename V_> class TLeftSignal,                          \
    template <typename D_, typename V_> class TRightSignal,                         \
    typename TLeftVal,                                                              \
    typename TRightVal,                                                             \
    class = std::enable_if<                                                         \
        IsSignal<D,TLeftSignal<D,TLeftVal>>::value>::type,                          \
    class = std::enable_if<                                                         \
        IsSignal<D,TRightSignal<D,TRightVal>>::value>::type                         \
>                                                                                   \
auto operator ## op(const TLeftSignal<D,TLeftVal>& lhs,                             \
                    const TRightSignal<D,TRightVal>& rhs)                           \
    -> Signal<D,                                                                   \
        typename std::remove_const<                                                 \
            decltype(std::declval<TLeftVal>() op std::declval<TRightVal>())>::type> \
{                                                                                   \
    using TRet = typename std::remove_const<                                        \
        decltype(std::declval<TLeftVal>() op std::declval<TRightVal>())>::type;     \
    return Signal<D,TRet>(                                                         \
        std::make_shared<REACT_IMPL::FunctionNode<D,TRet,TLeftVal,TRightVal>>(      \
            [] (const TLeftVal& a, const TRightVal& b) { return a op b; },          \
                lhs.GetPtr(), rhs.GetPtr(), false));                                \
}                                                                                   \
                                                                                    \
template                                                                            \
<                                                                                   \
    typename D,                                                                     \
    template <typename D_, typename V_> class TLeftSignal,                          \
    typename TLeftVal,                                                              \
    typename TRightVal,                                                             \
    class = std::enable_if<                                                         \
        IsSignal<D,TLeftSignal<D,TLeftVal>>::value>::type,                          \
    class = std::enable_if<                                                         \
        ! IsSignal<D,TRightVal>::value>::type                                       \
>                                                                                   \
auto operator ## op(const TLeftSignal<D,TLeftVal>& lhs,                             \
                    const TRightVal& rhs)                                           \
    -> Signal<D,                                                                   \
        typename std::remove_const<                                                 \
            decltype(std::declval<TLeftVal>() op rhs)>::type>                       \
{                                                                                   \
    using TRet = typename std::remove_const<                                        \
        decltype(std::declval<TLeftVal>() op rhs)>::type;                           \
    return Signal<D,TRet>(                                                         \
        std::make_shared<REACT_IMPL::FunctionNode<D,TRet,TLeftVal>>(                \
            [=] (const TLeftVal& a) { return a op rhs; }, lhs.GetPtr(), false));    \
}                                                                                   \
                                                                                    \
template                                                                            \
<                                                                                   \
    typename D,                                                                     \
    typename TLeftVal,                                                              \
    template <typename D_, typename V_> class TRightSignal,                         \
    typename TRightVal,                                                             \
    class = std::enable_if<                                                         \
        ! IsSignal<D,TRightVal>::value>::type,                                      \
    class = std::enable_if<                                                         \
        IsSignal<D,TRightSignal<D,TRightVal>>::value>::type                         \
>                                                                                   \
auto operator ## op(const TLeftVal& lhs,                                            \
                    const TRightSignal<D,TRightVal>& rhs)                           \
    -> Signal<D,                                                                   \
        typename std::remove_const<                                                 \
            decltype(lhs op std::declval<TRightVal>())>::type>                      \
{                                                                                   \
    using TRet = typename std::remove_const<                                        \
        decltype(lhs op std::declval<TRightVal>())>::type;                          \
    return Signal<D,TRet>(                                                         \
        std::make_shared<REACT_IMPL::FunctionNode<D,TRet,TRightVal>>(               \
            [=] (const TRightVal& a) { return lhs op a; }, rhs.GetPtr(), false));   \
}

DECLARE_OP(+);
DECLARE_OP(-);
DECLARE_OP(*);
DECLARE_OP(/);
DECLARE_OP(%);

DECLARE_OP(==);
DECLARE_OP(!=);
DECLARE_OP(<);
DECLARE_OP(<=);
DECLARE_OP(>);
DECLARE_OP(>=);

DECLARE_OP(&&);
DECLARE_OP(||);

#undef DECLARE_OP

//template
//<
//    typename D,                                                        
//    template <typename D_, typename V_> class TLeftSignal,            
//    template <typename D_, typename V_> class TRightSignal,            
//    typename TLeftVal,                                                
//    typename TRightVal,                                                
//    class = std::enable_if<                                            
//        IsSignal<D,TLeftSignal<D,TLeftVal>>::value>::type,            
//    class = std::enable_if<                                            
//        IsSignal<D,TRightSignal<D,TRightVal>>::value>::type        
//>                                                                    
//auto operator+(TLeftSignal<D,TLeftVal>&& lhs,
//               const TRightSignal<D,TRightVal>& rhs)
//    -> Signal<D,TLeftVal>
//{
//    lhs.GetPtr()->MergeOp(
//        rhs.GetPtr(), [] (const TLeftVal& a, const TRightVal& b) { return a + b; });
//    return std::move(lhs);
//}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// InputPack - Wraps several nodes in a tuple. Create with comma operator.
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename ... TValues
>
struct InputPack
{
    std::tuple<const Signal<D, TValues>& ...> Data;

    template <typename TFirstValue, typename TSecondValue>
    InputPack(const Signal<D,TFirstValue>& first, const Signal<D,TSecondValue>& second) :
        Data(std::tie(first, second))
    {
    }

    template <typename ... TCurValues, typename TAppendValue>
    InputPack(const InputPack<D, TCurValues ...>& curArgs, const Signal<D,TAppendValue>& newArg) :
        Data(std::tuple_cat(curArgs.Data, std::tie(newArg)))
    {
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Comma operator overload to create input pack from 2 nodes.
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename TLeftVal,
    typename TRightVal
>
auto operator,(const Signal<D,TLeftVal>& a, const Signal<D,TRightVal>& b)
    -> InputPack<D,TLeftVal, TRightVal>
{
    return InputPack<D, TLeftVal, TRightVal>(a, b);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Comma operator overload to append node to existing input pack.
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename ... TCurValues,
    typename TAppendValue
>
auto operator,(const InputPack<D, TCurValues ...>& cur, const Signal<D,TAppendValue>& append)
    -> InputPack<D,TCurValues ... , TAppendValue>
{
    return InputPack<D, TCurValues ... , TAppendValue>(cur, append);
}

/******************************************/ REACT_END /******************************************/

/***************************************/ REACT_IMPL_BEGIN /**************************************/

template
<
    typename D,
    typename F,
    typename ... TValues
>
struct ApplyHelper
{
    static inline auto MakeSignal(F&& func, const Signal<D,TValues>& ... args)
        -> Signal<D,decltype(func(std::declval<TValues>() ...))>
    {
        return D::MakeSignal(std::forward<F>(func), args ...);
    }
};

/****************************************/ REACT_IMPL_END /***************************************/

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// operator->* overload to connect inputs to a function and return the resulting node.
///////////////////////////////////////////////////////////////////////////////////////////////////
// Single input
template
<
    typename D,
    typename F,
    template <typename D_, typename V_> class TSignal,
    typename TValue,
    class = std::enable_if<
        IsSignal<D,TSignal<D,TValue>>::value>::type
>
auto operator->*(const TSignal<D,TValue>& inputNode, F&& func)
    -> Signal<D, typename std::result_of<F(TValue)>::type>
{
    return D::MakeSignal(std::forward<F>(func), inputNode);
}

// Multiple inputs
template
<
    typename D,
    typename F,
    typename ... TSignals
>
auto operator->*(const InputPack<D,TSignals ...>& inputPack, F&& func)
    -> Signal<D, typename std::result_of<F(TSignals ...)>::type>
{
    return apply(
        REACT_IMPL::ApplyHelper<D, F&&, TSignals ...>::MakeSignal,
        std::tuple_cat(std::forward_as_tuple(std::forward<F>(func)), inputPack.Data));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Flatten
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename TInnerValue
>
auto Flatten(const Signal<D,Signal<D,TInnerValue>>& node)
    -> Signal<D,TInnerValue>
{
    return Signal<D,TInnerValue>(
        std::make_shared<REACT_IMPL::FlattenNode<D, Signal<D,TInnerValue>, TInnerValue>>(
            node.GetPtr(), node.Value().GetPtr(), false));
}

/******************************************/ REACT_END /******************************************/
