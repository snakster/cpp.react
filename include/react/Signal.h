
//          Copyright Sebastian Jeckel 2016.
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
template <typename T>
class Signal;

template <typename T>
class VarSignal;

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
template <typename T>
class Signal : public REACT_IMPL::SignalBase<T>
{
private:
    using NodeType     = REACT_IMPL::SignalNode<T>;
    using NodePtrType  = std::shared_ptr<NodeType>;

public:
    using ValueType = T;

    // Default ctor
    Signal() = default;

    // Copy ctor
    Signal(const Signal&) = default;

    // Move ctor
    Signal(Signal&& other) = default;

    // Node ctor
    explicit Signal(NodePtrType&& nodePtr) :
        Signal::SignalBase( std::move(nodePtr) )
    {}

    // Copy assignment
    Signal& operator=(const Signal&) = default;

    // Move assignment
    Signal& operator=(Signal&& other) = default;

    const S& Get() const        { return Signal::SignalBase::getValue(); }
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

    template
    <
        typename ... Ts,
        typename FIn,
        typename F = typename std::decay<FIn>::type
    >
    static auto Create(FIn&& func, const Signal<TS>& ... args) -> Signal<S>
    {
        return Signal<S>(
            std::make_shared<REACT_IMPL::SignalFuncNode<S, F>>(
                std::forward<FIn>(func), GetNodePtr(args) ...));
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

    /// Create
    template <typename V>
    static auto Create(V&& value) -> VarSignal<S>
    {
        return VarSignal<S>(
            std::make_shared<REACT_IMPL::VarSignalNode<S>>(
                std::forward<V>(value)));
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