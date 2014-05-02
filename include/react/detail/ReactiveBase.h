
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/detail/Defs.h"

#include <functional>
#include <memory>
#include <utility>

#include "react/Traits.h"
#include "react/common/Types.h"
#include "react/detail/ReactiveInput.h"
#include "react/detail/Options.h"
#include "react/detail/graph/GraphBase.h"

#ifdef REACT_ENABLE_LOGGING
    #include "react/detail/logging/EventLog.h"
    #include "react/detail/logging/EventRecords.h"
#endif //REACT_ENABLE_LOGGING

#include "react/engine/ToposortEngine.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

template <typename L, typename R>
bool Equals(const L& lhs, const R& rhs)
{
    return lhs == rhs;
}

template <typename L, typename R>
bool Equals(const std::reference_wrapper<L>& lhs, const std::reference_wrapper<R>& rhs)
{
    return lhs.get() == rhs.get();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Reactive
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TNode>
class ReactiveBase
{
public:
    using DomainT   = typename TNode::DomainT;
    using NodePtrT  = SharedPtrT<TNode>;

    ReactiveBase() = default;
    ReactiveBase(const ReactiveBase&) = default;

    ReactiveBase(ReactiveBase&& other) :
        ptr_{ std::move(other.ptr_) }
    {}

    explicit ReactiveBase(const NodePtrT& ptr) :
        ptr_{ ptr }
    {}

    explicit ReactiveBase(NodePtrT&& ptr) :
        ptr_{ std::move(ptr) }
    {}

    const SharedPtrT<TNode>& NodePtr() const
    {
        return ptr_;
    }

    bool Equals(const ReactiveBase& other) const
    {
        return ptr_ == other.ptr_;
    }

    bool IsValid() const
    {
        return ptr_ != nullptr;
    }

protected:
    SharedPtrT<TNode>   ptr_;
};

/****************************************/ REACT_IMPL_END /***************************************/



/*****************************************/ REACT_BEGIN /*****************************************/

enum class EventToken;

template <typename D>
class ReactiveLoop;

template <typename D>
class Observer;

template <typename D, typename S>
class Signal;

template <typename D, typename S>
class VarSignal;

template <typename D, typename S, typename TOp>
class TempSignal;

template <typename D, typename E>
class Events;

template <typename D, typename E>
class EventSource;

template <typename D, typename E, typename TOp>
class TempEvents;

/******************************************/ REACT_END /******************************************/

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Domain
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename TEngine = ToposortEngine<sequential>
>
struct DomainPolicy
{
    using Engine = TEngine;
};

template <typename D, typename TPolicy>
class DomainBase
{
public:
    using TurnT = typename TPolicy::Engine::TurnT;

    DomainBase() = delete;

    using Policy = TPolicy;
    using Engine = EngineInterface<D, typename Policy::Engine>;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    /// Aliases for reactives of current domain
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename S>
    using SignalT = Signal<D,S>;

    template <typename S>
    using VarSignalT = VarSignal<D,S>;

    template <typename E = EventToken>
    using EventsT = Events<D,E>;

    template <typename E = EventToken>
    using EventSourceT = EventSource<D,E>;

    using ObserverT = Observer<D>;

    using ReactiveLoopT = ReactiveLoop<D>;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    /// MakeVar
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template
    <
        typename V,
        typename S = std::decay<V>::type,
        class = std::enable_if<
            !IsSignal<S>::value>::type
    >
    static auto MakeVar(V&& value)
        -> VarSignalT<S>
    {
        return REACT::MakeVar<D>(std::forward<V>(value));
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // MakeVar (higher order)
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    template
    <
        typename V,
        typename S = std::decay<V>::type,
        typename TInner = S::ValueT,
        class = std::enable_if<
            IsSignal<S>::value>::type
    >
    static auto MakeVar(V&& value)
        -> VarSignalT<SignalT<TInner>>
    {
        return REACT::MakeVar<D>(std::forward<V>(value));
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    /// MakeSignal
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template
    <
        typename FIn,
        typename ... TArgs,
        typename F = std::decay<FIn>::type,
        typename S = std::result_of<F(TArgs...)>::type,
        typename TOp = REACT_IMPL::FunctionOp<S,F,REACT_IMPL::SignalNodePtrT<D,TArgs> ...>
    >
    static auto MakeSignal(FIn&& func, const SignalT<TArgs>& ... args)
        -> TempSignal<D,S,TOp>
    {
        return REACT::MakeSignal<D>(std::forward<FIn>(func), args ...);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    /// MakeEventSource
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename E>
    static auto MakeEventSource()
        -> EventSourceT<E>
    {
        return REACT::MakeEventSource<D,E>();
    }

    static auto MakeEventSource()
        -> EventSourceT<EventToken>
    {
        return REACT::MakeEventSource<D>();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoTransaction
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename F>
    static void DoTransaction(F&& func)
    {
        InputManager<D>::DoTransaction(0, std::forward<F>(func));
    }

    template <typename F>
    static void DoTransaction(TurnFlagsT flags, F&& func)
    {
        InputManager<D>::DoTransaction(flags, std::forward<F>(func));
    }

#ifdef REACT_ENABLE_LOGGING
    ///////////////////////////////////////////////////////////////////////////////////////////////
    /// Log
    ///////////////////////////////////////////////////////////////////////////////////////////////
    static EventLog& Log()
    {
        static ObserverRegistry<D> instance;
        return instance;
    }
#endif //REACT_ENABLE_LOGGING
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
///// Ensure singletons are created immediately after domain declaration (TODO hax)
/////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class DomainInitializer
{
public:
    DomainInitializer()
    {
#ifdef REACT_ENABLE_LOGGING
        DomainSpecificData<D>::Log();
#endif //REACT_ENABLE_LOGGING

        typename D::Engine::Engine();
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ContinuationHolder
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class ContinuationHolder
{
public:
    using TurnT = typename D::TurnT;

    ContinuationHolder() = delete;

    static void                 SetTurn(TurnT& turn)    { ptr_ = &turn.continuation_; }
    static void                 Clear()                 { ptr_ = nullptr; }
    static ContinuationInput*   Get()                   { return ptr_; }

private:
    static REACT_TLS ContinuationInput* ptr_;
};

template <typename D>
ContinuationInput* ContinuationHolder<D>::ptr_(nullptr);

/****************************************/ REACT_IMPL_END /***************************************/

#define REACTIVE_DOMAIN(name, ...) \
    struct name : public REACT_IMPL::DomainBase<name, REACT_IMPL::DomainPolicy<__VA_ARGS__ >> {}; \
    REACT_IMPL::DomainInitializer< name > name ## _initializer_;
