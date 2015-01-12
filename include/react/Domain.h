
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DOMAIN_H_INCLUDED
#define REACT_DOMAIN_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <memory>
#include <utility>

#include "react/detail/DomainBase.h"
#include "react/detail/ReactiveInput.h"

#include "react/detail/graph/ContinuationNodes.h"

#ifdef REACT_ENABLE_LOGGING
    #include "react/logging/EventLog.h"
    #include "react/logging/EventRecords.h"
#endif //REACT_ENABLE_LOGGING

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

template <typename D, typename E>
class Events;

template <typename D, typename E>
class EventSource;

template <typename D, typename E, typename TOp>
class TempEvents;

enum class Token;

template <typename D>
class Observer;

template <typename D>
class ScopedObserver;

template <typename D>
class Reactor;

template <typename D, typename ... TValues>
class SignalPack;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Common types & constants
///////////////////////////////////////////////////////////////////////////////////////////////////
using REACT_IMPL::TransactionFlagsT;

// ETransactionFlags
using REACT_IMPL::ETransactionFlags;
using REACT_IMPL::allow_merging;

#ifdef REACT_ENABLE_LOGGING
    using REACT_IMPL::EventLog;
#endif //REACT_ENABLE_LOGGING

// Domain modes
using REACT_IMPL::EDomainMode;
using REACT_IMPL::sequential;
using REACT_IMPL::sequential_concurrent;
using REACT_IMPL::parallel;
using REACT_IMPL::parallel_concurrent;

// Expose enum type so aliases for engines can be declared, but don't
// expose the actual enum values as they are reserved for internal use.
using REACT_IMPL::EPropagationMode;

using REACT_IMPL::WeightHint;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// TransactionStatus
///////////////////////////////////////////////////////////////////////////////////////////////////
class TransactionStatus
{
    using StateT = REACT_IMPL::SharedWaitingState;
    using PtrT = REACT_IMPL::WaitingStatePtrT;

public:
    // Default ctor
    inline TransactionStatus() :
        statePtr_( StateT::Create() )
    {}

    // Move ctor
    inline TransactionStatus(TransactionStatus&& other) :
        statePtr_( std::move(other.statePtr_) )
    {
        other.statePtr_ = StateT::Create();
    }

    // Move assignment
    inline TransactionStatus& operator=(TransactionStatus&& other)
    {
        if (this != &other)
        {
            statePtr_ = std::move(other.statePtr_);
            other.statePtr_ = StateT::Create();
        }
        return *this;
    }

    // Deleted copy ctor & assignment
    TransactionStatus(const TransactionStatus&) = delete;
    TransactionStatus& operator=(const TransactionStatus&) = delete;

    inline void Wait()
    {
        assert(statePtr_.Get() != nullptr);
        statePtr_->Wait();
    }

private:
    PtrT statePtr_;

    template <typename D, typename F>
    friend void AsyncTransaction(TransactionStatus& status, F&& func);

    template <typename D, typename F>
    friend void AsyncTransaction(TransactionFlagsT flags, TransactionStatus& status, F&& func);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Continuation
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename D2 = D
>
class Continuation : public REACT_IMPL::ContinuationBase<D,D2>
{
private:
    using NodePtrT  = REACT_IMPL::NodeBasePtrT<D>;

public:
    using SourceDomainT = D;
    using TargetDomainT = D2;

    // Default ctor
    Continuation() = default;

    // Move ctor
    Continuation(Continuation&& other) :
        Continuation::ContinuationBase( std::move(other) )
    {}

    // Node ctor
    explicit Continuation(NodePtrT&& nodePtr) :
        Continuation::ContinuationBase( std::move(nodePtr) )
    {}

    // Move assignment
    Continuation& operator=(Continuation&& other)
    {
        Continuation::ContinuationBase::operator=( std::move(other) );
        return *this;
    }

    // Deleted copy ctor & assignment
    Continuation(const Continuation&) = delete;
    Continuation& operator=(const Continuation&) = delete;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MakeContinuation - Signals
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename DOut = D,
    typename S,
    typename FIn
>
auto MakeContinuation(TransactionFlagsT flags, const Signal<D,S>& trigger, FIn&& func)
    -> Continuation<D,DOut>
{
    static_assert(DOut::is_concurrent,
        "MakeContinuation: Target domain does not support concurrent input.");

    using REACT_IMPL::SignalContinuationNode;
    using F = typename std::decay<FIn>::type;

    return Continuation<D,DOut>(
        std::make_shared<SignalContinuationNode<D,DOut,S,F>>(
            flags, GetNodePtr(trigger), std::forward<FIn>(func)));
}

template
<
    typename D,
    typename DOut = D,
    typename S,
    typename FIn
>
auto MakeContinuation(const Signal<D,S>& trigger, FIn&& func)
    -> Continuation<D,DOut>
{
    return MakeContinuation<D,DOut>(0, trigger, std::forward<FIn>(func));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MakeContinuation - Events
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename DOut = D,
    typename E,
    typename FIn
>
auto MakeContinuation(TransactionFlagsT flags, const Events<D,E>& trigger, FIn&& func)
    -> Continuation<D,DOut>
{
    static_assert(DOut::is_concurrent,
        "MakeContinuation: Target domain does not support concurrent input.");

    using REACT_IMPL::EventContinuationNode;
    using REACT_IMPL::AddContinuationRangeWrapper;
    using REACT_IMPL::IsCallableWith;
    using REACT_IMPL::EventRange;

    using F = typename std::decay<FIn>::type;

    using WrapperT =
        typename std::conditional<
            IsCallableWith<F,void, EventRange<E>>::value,
            F,
            typename std::conditional<
                IsCallableWith<F, void, E>::value,
                AddContinuationRangeWrapper<E, F>,
                void
            >::type
        >::type;

    static_assert(! std::is_same<WrapperT,void>::value,
        "MakeContinuation: Passed function does not match any of the supported signatures.");

    return Continuation<D,DOut>(
        std::make_shared<EventContinuationNode<D,DOut,E,WrapperT>>(
            flags, GetNodePtr(trigger), std::forward<FIn>(func)));
}

template
<
    typename D,
    typename DOut = D,
    typename E,
    typename FIn
>
auto MakeContinuation(const Events<D,E>& trigger, FIn&& func)
    -> Continuation<D,DOut>
{
    return MakeContinuation<D,DOut>(0, trigger, std::forward<FIn>(func));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MakeContinuation - Synced
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename DOut = D,
    typename E,
    typename FIn,
    typename ... TDepValues
>
auto MakeContinuation(TransactionFlagsT flags, const Events<D,E>& trigger,
                      const SignalPack<D,TDepValues...>& depPack, FIn&& func)
    -> Continuation<D,DOut>
{
    static_assert(DOut::is_concurrent,
        "MakeContinuation: Target domain does not support concurrent input.");

    using REACT_IMPL::SyncedContinuationNode;
    using REACT_IMPL::AddContinuationRangeWrapper;
    using REACT_IMPL::IsCallableWith;
    using REACT_IMPL::EventRange;

    using F = typename std::decay<FIn>::type;

    using WrapperT =
        typename std::conditional<
            IsCallableWith<F, void, EventRange<E>, TDepValues ...>::value,
            F,
            typename std::conditional<
                IsCallableWith<F, void, E, TDepValues ...>::value,
                AddContinuationRangeWrapper<E, F, TDepValues ...>,
                void
            >::type
        >::type;

    static_assert(! std::is_same<WrapperT,void>::value,
        "MakeContinuation: Passed function does not match any of the supported signatures.");

    struct NodeBuilder_
    {
        NodeBuilder_(TransactionFlagsT flags, const Events<D,E>& trigger, FIn&& func) :
            MyFlags( flags ),
            MyTrigger( trigger ),
            MyFunc( std::forward<FIn>(func) )
        {}

        auto operator()(const Signal<D,TDepValues>& ... deps)
            -> Continuation<D,DOut>
        {
            return Continuation<D,DOut>(
                std::make_shared<SyncedContinuationNode<D,DOut,E,WrapperT,TDepValues...>>(
                    MyFlags,
                    GetNodePtr(MyTrigger),
                    std::forward<FIn>(MyFunc), GetNodePtr(deps) ...));
        }

        TransactionFlagsT   MyFlags;
        const Events<D,E>&  MyTrigger;
        FIn                 MyFunc;
    };

    return REACT_IMPL::apply(
        NodeBuilder_( flags, trigger, std::forward<FIn>(func) ),
        depPack.Data);
}

template
<
    typename D,
    typename DOut = D,
    typename E,
    typename FIn,
    typename ... TDepValues
>
auto MakeContinuation(const Events<D,E>& trigger,
                      const SignalPack<D,TDepValues...>& depPack, FIn&& func)
    -> Continuation<D,DOut>
{
    return MakeContinuation<D,DOut>(0, trigger, depPack, std::forward<FIn>(func));
}

///////////////////////////////////////////////////////////////////////////////////////////////
/// DoTransaction
///////////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename F>
void DoTransaction(F&& func)
{
    using REACT_IMPL::DomainSpecificInputManager;
    DomainSpecificInputManager<D>::Instance().DoTransaction(0, std::forward<F>(func));
}

template <typename D, typename F>
void DoTransaction(TransactionFlagsT flags, F&& func)
{
    using REACT_IMPL::DomainSpecificInputManager;
    DomainSpecificInputManager<D>::Instance().DoTransaction(flags, std::forward<F>(func));
}

///////////////////////////////////////////////////////////////////////////////////////////////
/// AsyncTransaction
///////////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename F>
void AsyncTransaction(F&& func)
{
    static_assert(D::is_concurrent,
        "AsyncTransaction: Domain does not support concurrent input.");

    using REACT_IMPL::DomainSpecificInputManager;
    DomainSpecificInputManager<D>::Instance()
        .AsyncTransaction(0, nullptr, std::forward<F>(func));
}

template <typename D, typename F>
void AsyncTransaction(TransactionFlagsT flags, F&& func)
{
    static_assert(D::is_concurrent,
        "AsyncTransaction: Domain does not support concurrent input.");

    using REACT_IMPL::DomainSpecificInputManager;
    DomainSpecificInputManager<D>::Instance()
        .AsyncTransaction(flags, nullptr, std::forward<F>(func));
}

template <typename D, typename F>
void AsyncTransaction(TransactionStatus& status, F&& func)
{
    static_assert(D::is_concurrent,
        "AsyncTransaction: Domain does not support concurrent input.");

    using REACT_IMPL::DomainSpecificInputManager;

    DomainSpecificInputManager<D>::Instance()
        .AsyncTransaction(0, status.statePtr_, std::forward<F>(func));
}

template <typename D, typename F>
void AsyncTransaction(TransactionFlagsT flags, TransactionStatus& status, F&& func)
{
    static_assert(D::is_concurrent,
        "AsyncTransaction: Domain does not support concurrent input.");

    using REACT_IMPL::DomainSpecificInputManager;
    DomainSpecificInputManager<D>::Instance()
        .AsyncTransaction(flags, status.statePtr_, std::forward<F>(func));
}

/******************************************/ REACT_END /******************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Domain definition macro
///////////////////////////////////////////////////////////////////////////////////////////////////
#define REACTIVE_DOMAIN(name, ...)                                                          \
    struct name :                                                                           \
        public REACT_IMPL::DomainBase<name, REACT_IMPL::DomainPolicy< __VA_ARGS__ >> {};    \
    static REACT_IMPL::DomainInitializer<name> name ## _initializer_;

/*
    A brief reminder why the domain initializer is here:
    Each domain has a couple of singletons (debug log, engine, input manager) which are
    currently implemented as meyer singletons. From what I understand, these are thread-safe
    in C++11, but not all compilers implement that yet. That's why a static initializer has
    been added to make sure singleton creation happens before any multi-threaded access.
    This implemenation is obviously inconsequential.
 */

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Define type aliases for given domain
///////////////////////////////////////////////////////////////////////////////////////////////////
#define USING_REACTIVE_DOMAIN(name)                                                         \
    template <typename S>                                                                   \
    using SignalT = Signal<name,S>;                                                         \
                                                                                            \
    template <typename S>                                                                   \
    using VarSignalT = VarSignal<name,S>;                                                   \
                                                                                            \
    template <typename E = Token>                                                           \
    using EventsT = Events<name,E>;                                                         \
                                                                                            \
    template <typename E = Token>                                                           \
    using EventSourceT = EventSource<name,E>;                                               \
                                                                                            \
    using ObserverT = Observer<name>;                                                       \
                                                                                            \
    using ScopedObserverT = ScopedObserver<name>;                                           \
                                                                                            \
    using ReactorT = Reactor<name>;

#endif // REACT_DOMAIN_H_INCLUDED