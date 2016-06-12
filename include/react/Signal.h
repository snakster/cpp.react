
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
template <typename S>
class Signal;

template <typename S>
class VarSignal;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MakeVar
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename V,
    typename S = typename std::decay<V>::type,
    class = typename std::enable_if<!IsSignalType<S>::value>::type,
    class = typename std::enable_if<!IsEventType<S>::value>::type
>
auto MakeVar(V&& value) -> VarSignal<S>
{
    return VarSignal<S>(
        std::make_shared<REACT_IMPL::VarNode<S>>(
            std::forward<V>(value)));
}

template <typename S>
auto MakeVar(std::reference_wrapper<S> value) -> VarSignal<S&>
{
    return VarSignal<S&>(
        std::make_shared<REACT_IMPL::VarNode<std::reference_wrapper<S>>>(value));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MakeVar (higher order reactives)
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename V,
    typename S = typename std::decay<V>::type,
    typename TInner = typename S::ValueT,
    class = typename std::enable_if<IsSignal<S>::value>::type
>
auto MakeVar(V&& value)
    -> VarSignal<Signal<TInner>>
{
    return VarSignal<D,Signal<D,TInner>>(
        std::make_shared<REACT_IMPL::VarNode<D,Signal<D,TInner>>>(
            std::forward<V>(value)));
}

template
<
    
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
/// Flatten
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TInnerValue>
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
template <typename S>
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

///////////////////////////////////////////////////////////////////////////////////////////////////
/// VarSignal
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class VarSignal : public Signal<S>
{
private:
    using NodeT     = REACT_IMPL::VarNode<S>;
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

/******************************************/ REACT_END /******************************************/

/***************************************/ REACT_IMPL_BEGIN /**************************************/

template <typename L, typename R>
bool Equals(const Signal<L>& lhs, const Signal<R>& rhs)
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