
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
    {}
    
    explicit Signal(const std::shared_ptr<NodeT>& ptr) :
        Reactive(ptr)
    {}

    explicit Signal(std::shared_ptr<NodeT>&& ptr) :
        Reactive(std::move(ptr))
    {}

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
    {}
    
    explicit VarSignal(const std::shared_ptr<NodeT>& ptr) :
        Signal(ptr)
    {}

    explicit VarSignal(std::shared_ptr<NodeT>&& ptr) :
        Signal(std::move(ptr))
    {}

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
protected:
    using NodeT = REACT_IMPL::OpSignalNode<D,S,TOp>;

public:    
    TempSignal() :
        Signal()
    {}

    explicit TempSignal(const std::shared_ptr<NodeT>& ptr) :
        Signal(ptr)
    {}

    explicit TempSignal(std::shared_ptr<NodeT>&& ptr) :
        Signal(std::move(ptr))
    {}

    TOp StealOp()
    {
        return std::move(std::static_pointer_cast<NodeT>(ptr_)->StealOp());
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
            std::forward<V>(value)));
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
            std::forward<V>(value)));
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
            std::forward<V>(value)));
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
            std::forward<V>(value)));
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
auto MakeSignal_old(F&& func, const Signal<D,TArgs>& ... args)
    -> Signal<D, typename std::result_of<F(TArgs...)>::type>
{
    static_assert(sizeof...(TArgs) > 0,
        "react::MakeSignal requires at least 1 signal dependency.");

    using S = typename std::result_of<F(TArgs...)>::type;

    return Signal<D,S>(
        std::make_shared<REACT_IMPL::FunctionNode<D,S,F,TArgs ...>>(
            std::forward<F>(func), args.GetPtr() ...));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MakeSignal
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename F,
    typename ... TArgs,
    typename S,
    typename TOp
>
auto MakeSignal(F&& func, const Signal<D,TArgs>& ... args)
    -> TempSignal<D,S,TOp>
{
    static_assert(sizeof...(TArgs) > 0,
        "react::MakeSignal requires at least 1 signal dependency.");

    return TempSignal<D,S,TOp>(
        std::make_shared<REACT_IMPL::OpSignalNode<D,S,TOp>>(
            std::forward<F>(func), args.GetPtr() ...  ));
}

//template
//<
//    typename D,
//    template <typename D_, typename V_> class TRightSignal,
//    typename TLeftVal,
//    typename TLeftOp,
//    typename TRightVal,
//    typename S = PlusFunctor<TLeftVal,TRightVal>::RetT,
//    typename TOp = REACT_IMPL::FunctionOp<S,PlusFunctor<TLeftVal,TRightVal>, REACT_IMPL::SignalNodePtr<D,TArgs> ...>,
//    class = std::enable_if<
//        IsSignal<D,TRightSignal<D,TRightVal>>::value>::type
//>
//auto operator+(TempSignal<D,TLeftVal,TLeftOp>&& lhs,
//               const TRightSignal<D,TRightVal>& rhs)
//    -> TempSignal<D,TLeftVal,TOp>
//{
//    return TempSignal<D,S,TOp>(
//        std::make_shared<REACT_IMPL::OpSignalNode<D,S,TOp>>(
//            PlusFunctor<TLeftVal,TRightVal>(), lhs.StealOp(), rhs.GetPtr()));
//}

//template <typename T>                                                           
//struct NegationOpFunctor                                                        
//{                                                                               
//    using RetT = decltype(! std::declval<T>());                                  
//                                                                                
//    RetT operator()(const T& v) const                                           
//    {                                                                           
//        return ! v;                                                            
//    }                                                                           
//};                                                                              
//                                                                                
//template                                                                        
//<                                                                               
//    typename D,                                                                 
//    template <typename D_, typename V_> class TSignal,                          
//    typename TVal,                                                              
//    typename F = NegationOpFunctor<TVal>,                                       
//    typename S = F::RetT,
//    typename TOp = REACT_IMPL::FunctionOp<S,F,REACT_IMPL::SignalNodePtr<D,TVal>>,
//    class = std::enable_if<                                                     
//        IsSignal<D,TSignal<D,TVal>>::value>::type                               
//>                                                                               
//auto operator!(const TSignal<D,TVal>& arg)
//    -> TempSignal<D,S,TOp>
//{
//    return TempSignal<D,S,TOp>(
//        std::make_shared<REACT_IMPL::OpSignalNode<D,S,TOp>>(
//            F(), arg.GetPtr()));
//}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Unary operators
///////////////////////////////////////////////////////////////////////////////////////////////////
#define DECLARE_OP(op,name)                                                         \
template <typename T>                                                               \
struct name ## OpFunctor                                                            \
{                                                                                   \
    T operator()(const T& v) const { return op v; }                                 \
};                                                                                  \
                                                                                    \
template                                                                            \
<                                                                                   \
    typename TSignal,                                                               \
    typename D = TSignal::DomainT,                                                  \
    typename TVal = TSignal::ValueT,                                                \
    typename F = name ## OpFunctor<TVal>,                                           \
    typename S = std::result_of<F(TVal)>::type,                                     \
    typename TOp = REACT_IMPL::FunctionOp<S,F,REACT_IMPL::SignalNodePtr<D,TVal>>,   \
    class = std::enable_if<                                                         \
        IsSignal<TSignal>::value>::type                                             \
>                                                                                   \
auto operator ## op(const TSignal& arg)                                             \
    -> TempSignal<D,S,TOp>                                                          \
{                                                                                   \
    return TempSignal<D,S,TOp>(                                                     \
        std::make_shared<REACT_IMPL::OpSignalNode<D,S,TOp>>(                        \
            F(), arg.GetPtr()));                                                    \
}

DECLARE_OP(+, UnaryPlus);
DECLARE_OP(-, UnaryMinus);
DECLARE_OP(!, LogicalNegation);
//DECLARE_OP(++, Increment);
//DECLARE_OP(--, Decrement);

#undef DECLARE_OP 

/*template <typename L, typename R>                                                   
struct AdditionOpFunctor                                                            
{                                                                                   
    L operator()(const L& lhs, const R& rhs) const { return lhs + rhs; }        
};

template                                                                            
<                                                                                   
    typename TLeftSignal,                          
    typename TRightSignal,                         
    typename D = TLeftSignal::DomainT,                                                                     
    typename TLeftVal = TLeftSignal::ValueT,                                                              
    typename TRightVal = TRightSignal::ValueT,                                                             
    typename F = AdditionOpFunctor<TLeftVal,TRightVal>,                             
    typename S = std::result_of<F(TLeftVal,TRightVal)>::type,
    typename TOp = REACT_IMPL::FunctionOp<S,F,
        REACT_IMPL::SignalNodePtr<D,TLeftVal>,
        REACT_IMPL::SignalNodePtr<D,TRightVal>>,   
    class = std::enable_if<                                                         
        IsSignal<TLeftSignal>::value>::type,                          
    class = std::enable_if<                                                         
        IsSignal<TRightSignal>::value>::type                         
>                                                                                   
auto operator+(const TLeftSignal& lhs,                             
               const TRightSignal& rhs)                           
    -> TempSignal<D,S,TOp>                                                          
{

    return TempSignal<D,S,TOp>(                                                     
        std::make_shared<REACT_IMPL::OpSignalNode<D,S,TOp>>(                        
            F(), lhs.GetPtr(), rhs.GetPtr()));                                      
}

template <typename L, typename R>                                                   
struct AdditionOpLFunctor                                                            
{                                                                                       
    AdditionOpLFunctor(AdditionOpLFunctor&& other) :
        RightVal{ std::move(other.RightVal) }
    {}

    template <typename T>
    AdditionOpLFunctor(T&& val) :
        RightVal{ std::forward<T>(val) }
    {}

    AdditionOpLFunctor(const AdditionOpLFunctor& other) = delete;

    L operator()(const L& lhs) const
    {
        return lhs + RightVal;
    }

    R RightVal;
};

template                                                                            
<
    typename TLeftSignal,                          
    typename TRightValIn,                                                             
    typename D = TLeftSignal::DomainT,                                                                     
    typename TLeftVal = TLeftSignal::ValueT,                                                              
    typename TRightVal = std::decay<TRightValIn>::type,                                                  
    typename F = AdditionOpLFunctor<TLeftVal,TRightVal>,                             
    typename S = std::result_of<F(TLeftVal)>::type,
    typename TOp = REACT_IMPL::FunctionOp<S,F,
        REACT_IMPL::SignalNodePtr<D,TLeftVal>>,
    class = std::enable_if<                                                         
        IsSignal<TLeftSignal>::value>::type,                          
    class = std::enable_if<                                                         
        ! IsSignal<TRightVal>::value>::type
>                                                                                   
auto operator+(const TLeftSignal& lhs, TRightValIn&& rhs)                                           
    -> TempSignal<D,S,TOp>                                                                  
{
    return TempSignal<D,S,TOp>(                                                     
        std::make_shared<REACT_IMPL::OpSignalNode<D,S,TOp>>(                        
            F(std::forward<TRightValIn>(rhs)), lhs.GetPtr()));
}

template <typename L, typename R>                                                   
struct AdditionOpRFunctor                                                            
{                                                                                       
    AdditionOpRFunctor(AdditionOpRFunctor&& other) :
        LeftVal{ std::move(other.LeftVal) }
    {}

    template <typename T>
    AdditionOpRFunctor(T&& val) :
        LeftVal{ std::forward<T>(val) }
    {}

    AdditionOpRFunctor(const AdditionOpLFunctor& other) = delete;

    R operator()(const R& rhs) const
    {
        return LeftVal + rhs;
    }

    L LeftVal;
};

template                                                                            
<   
    typename TLeftValIn,                                                              
    typename TRightSignal,                         
    typename D = TRightSignal::DomainT,                                                                     
    typename TLeftVal = std::decay<TLeftValIn>::type,                                                  
    typename TRightVal = TRightSignal::ValueT, 
    typename F = AdditionOpRFunctor<TLeftVal,TRightVal>,                             
    typename S = std::result_of<F(TRightVal)>::type,
    typename TOp = REACT_IMPL::FunctionOp<S,F,
        REACT_IMPL::SignalNodePtr<D,TRightVal>>,
    class = std::enable_if<                                                         
        ! IsSignal<TLeftVal>::value>::type,                                      
    class = std::enable_if<                                                         
        IsSignal<TRightSignal>::value>::type                         
>                                                                                   
auto operator+(TLeftValIn&& lhs,                                            
                const TRightSignal& rhs)                           
    -> TempSignal<D,S,TOp>                                                                  
{                                                                                   
    return TempSignal<D,S,TOp>(                                                     
        std::make_shared<REACT_IMPL::OpSignalNode<D,S,TOp>>(                        
            F(std::forward<TLeftValIn>(lhs)), rhs.GetPtr()));                                                     
}    */                                                                       

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Binary operators
///////////////////////////////////////////////////////////////////////////////////////////////////
#define DECLARE_OP(op,name)                                                         \
template <typename L, typename R>                                                   \
struct name ## OpFunctor                                                            \
{                                                                                   \
    L operator()(const L& lhs, const R& rhs) const { return lhs op rhs; }           \
};                                                                                  \
                                                                                    \
template                                                                            \
<                                                                                   \
    typename TLeftSignal,                                                           \
    typename TRightSignal,                                                          \
    typename D = TLeftSignal::DomainT,                                              \
    typename TLeftVal = TLeftSignal::ValueT,                                        \
    typename TRightVal = TRightSignal::ValueT,                                      \
    typename F = name ## OpFunctor<TLeftVal,TRightVal>,                             \
    typename S = std::result_of<F(TLeftVal,TRightVal)>::type,                       \
    typename TOp = REACT_IMPL::FunctionOp<S,F,                                      \
        REACT_IMPL::SignalNodePtr<D,TLeftVal>,                                      \
        REACT_IMPL::SignalNodePtr<D,TRightVal>>,                                    \
    class = std::enable_if<                                                         \
        IsSignal<TLeftSignal>::value>::type,                                        \
    class = std::enable_if<                                                         \
        IsSignal<TRightSignal>::value>::type                                        \
>                                                                                   \
auto operator ## op(const TLeftSignal& lhs, const TRightSignal& rhs)                \
    -> TempSignal<D,S,TOp>                                                          \
{                                                                                   \
    return TempSignal<D,S,TOp>(                                                     \
        std::make_shared<REACT_IMPL::OpSignalNode<D,S,TOp>>(                        \
            F(), lhs.GetPtr(), rhs.GetPtr()));                                      \
}                                                                                   \
                                                                                    \
template <typename L, typename R>                                                   \
struct name ## OpLFunctor                                                           \
{                                                                                   \
    name ## OpLFunctor(name ## OpLFunctor&& other) :                                \
        RightVal{ std::move(other.RightVal) }                                       \
    {}                                                                              \
                                                                                    \
    template <typename T>                                                           \
    name ## OpLFunctor(T&& val) :                                                   \
        RightVal{ std::forward<T>(val) }                                            \
    {}                                                                              \
                                                                                    \
    name ## OpLFunctor(const name ## OpLFunctor& other) = delete;                   \
                                                                                    \
    L operator()(const L& lhs) const { return lhs op RightVal; }                    \
                                                                                    \
    R RightVal;                                                                     \
};                                                                                  \
                                                                                    \
template                                                                            \
<                                                                                   \
    typename TLeftSignal,                                                           \
    typename TRightValIn,                                                           \
    typename D = TLeftSignal::DomainT,                                              \
    typename TLeftVal = TLeftSignal::ValueT,                                        \
    typename TRightVal = std::decay<TRightValIn>::type,                             \
    typename F = name ## OpLFunctor<TLeftVal,TRightVal>,                            \
    typename S = std::result_of<F(TLeftVal)>::type,                                 \
    typename TOp = REACT_IMPL::FunctionOp<S,F,                                      \
                   REACT_IMPL::SignalNodePtr<D,TLeftVal>>,                          \
    class = std::enable_if<                                                         \
        IsSignal<TLeftSignal>::value>::type,                                        \
    class = std::enable_if<                                                         \
        ! IsSignal<TRightVal>::value>::type                                         \
>                                                                                   \
auto operator ## op(const TLeftSignal& lhs, TRightValIn&& rhs)                      \
    -> TempSignal<D,S,TOp>                                                          \
{                                                                                   \
    return TempSignal<D,S,TOp>(                                                     \
        std::make_shared<REACT_IMPL::OpSignalNode<D,S,TOp>>(                        \
            F(std::forward<TRightValIn>(rhs)), lhs.GetPtr()));                      \
}                                                                                   \
                                                                                    \
template <typename L, typename R>                                                   \
struct name ## OpRFunctor                                                           \
{                                                                                   \
    name ## OpRFunctor(name ## OpRFunctor&& other) :                                \
        LeftVal{ std::move(other.LeftVal) }                                         \
    {}                                                                              \
                                                                                    \
    template <typename T>                                                           \
    name ## OpRFunctor(T&& val) :                                                   \
        LeftVal{ std::forward<T>(val) }                                             \
    {}                                                                              \
                                                                                    \
    name ## OpRFunctor(const name ## OpRFunctor& other) = delete;                   \
                                                                                    \
    R operator()(const R& rhs) const { return LeftVal op rhs; }                     \
                                                                                    \
    L LeftVal;                                                                      \
};                                                                                  \
                                                                                    \
template                                                                            \
<                                                                                   \
    typename TLeftValIn,                                                            \
    typename TRightSignal,                                                          \
    typename D = TRightSignal::DomainT,                                             \
    typename TLeftVal = std::decay<TLeftValIn>::type,                               \
    typename TRightVal = TRightSignal::ValueT,                                      \
    typename F = name ## OpRFunctor<TLeftVal,TRightVal>,                            \
    typename S = std::result_of<F(TRightVal)>::type,                                \
    typename TOp = REACT_IMPL::FunctionOp<S,F,                                      \
        REACT_IMPL::SignalNodePtr<D,TRightVal>>,                                    \
    class = std::enable_if<                                                         \
        ! IsSignal<TLeftVal>::value>::type,                                         \
    class = std::enable_if<                                                         \
        IsSignal<TRightSignal>::value>::type                                        \
>                                                                                   \
auto operator ## op(TLeftValIn&& lhs, const TRightSignal& rhs)                      \
    -> TempSignal<D,S,TOp>                                                          \
{                                                                                   \
    return TempSignal<D,S,TOp>(                                                     \
        std::make_shared<REACT_IMPL::OpSignalNode<D,S,TOp>>(                        \
            F(std::forward<TLeftValIn>(lhs)), rhs.GetPtr()));                       \
}

DECLARE_OP(+, Addition);
DECLARE_OP(-, Subtraction);
DECLARE_OP(*, Multiplication);
DECLARE_OP(/, Division);
DECLARE_OP(%, Modulo);

DECLARE_OP(==, Equal);
DECLARE_OP(!=, NotEqual);
DECLARE_OP(<, Less);
DECLARE_OP(<=, LessEqual);
DECLARE_OP(>, Greater);
DECLARE_OP(>=, GreaterEqual);

DECLARE_OP(&&, LogicalAnd);
DECLARE_OP(||, LogicalOr);

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
        IsSignal<TSignal<D,TValue>>::value>::type
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
            node.GetPtr(), node.Value().GetPtr()));
}

/******************************************/ REACT_END /******************************************/
