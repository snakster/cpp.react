
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

#include "react/detail/IReactiveEngine.h"
#include "react/detail/ReactiveInput.h"
#include "react/detail/graph/ContinuationNodes.h"

#ifdef REACT_ENABLE_LOGGING
    #include "react/logging/EventLog.h"
    #include "react/logging/EventRecords.h"
#endif //REACT_ENABLE_LOGGING

#include "react/engine/ToposortEngine.h"

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

using REACT_IMPL::TurnFlagsT;

//ETurnFlags
using REACT_IMPL::enable_input_merging;

#ifdef REACT_ENABLE_LOGGING
    using REACT_IMPL::EventLog;
#endif //REACT_ENABLE_LOGGING    

///////////////////////////////////////////////////////////////////////////////////////////////////
/// TransactionStatus
///////////////////////////////////////////////////////////////////////////////////////////////////
class TransactionStatus
{
    using StateT = REACT_IMPL::AsyncState;

public:
    TransactionStatus() :
        state_( std::make_shared<StateT>() )
    {}

    TransactionStatus(const TransactionStatus&) = default;

    TransactionStatus(TransactionStatus&& other) :
        state_( std::move(other.state_) )
    {}

    TransactionStatus& operator=(const TransactionStatus&) = default;

    TransactionStatus& operator=(TransactionStatus&& other)
    {
        state_ = std::move(other.state_);
        return *this;
    }

    inline void Wait()
    {
        assert(state_.get() != nullptr);
        state_->Wait();
    }

private:
    std::shared_ptr<StateT> state_;

    template <typename D, typename TPolicy>
    friend class DomainBase;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// DomainBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename TPolicy>
class DomainBase
{
public:
    using TurnT = typename TPolicy::Engine::TurnT;

    DomainBase() = delete;

    using Policy = TPolicy;
    using Engine = REACT_IMPL::EngineInterface<D, typename Policy::Engine>;

    ///////////////////////////////////////////////////////////////////////////////////////////////
    /// Domain traits
    ///////////////////////////////////////////////////////////////////////////////////////////////
    static const bool uses_node_update_timer =
        REACT_IMPL::NodeUpdateTimerEnabled<typename Policy::Engine>::value;

    static const bool is_concurrent =
        REACT_IMPL::IsConcurrentEngine<typename Policy::Engine>::value;

    static const bool is_parallel =
        REACT_IMPL::IsParallelEngine<typename Policy::Engine>::value;

    ///////////////////////////////////////////////////////////////////////////////////////////////
    /// Aliases for reactives of this domain
    ///////////////////////////////////////////////////////////////////////////////////////////////
    template <typename S>
    using SignalT = Signal<D,S>;

    template <typename S>
    using VarSignalT = VarSignal<D,S>;

    template <typename E = Token>
    using EventsT = Events<D,E>;

    template <typename E = Token>
    using EventSourceT = EventSource<D,E>;

    using ObserverT = Observer<D>;

    using ScopedObserverT = ScopedObserver<D>;

    using ReactorT = Reactor<D>;

    ///////////////////////////////////////////////////////////////////////////////////////////////
    /// DoTransaction
    ///////////////////////////////////////////////////////////////////////////////////////////////
    template <typename F>
    static void DoTransaction(F&& func)
    {
        using REACT_IMPL::DomainSpecificInputManager;
        DomainSpecificInputManager<D>::Instance().DoTransaction(0, std::forward<F>(func));
    }

    template <typename F>
    static void DoTransaction(TurnFlagsT flags, F&& func)
    {
        using REACT_IMPL::DomainSpecificInputManager;
        DomainSpecificInputManager<D>::Instance().DoTransaction(flags, std::forward<F>(func));
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    /// AsyncTransaction
    ///////////////////////////////////////////////////////////////////////////////////////////////
    template <typename F>
    static void AsyncTransaction(F&& func)
    {
        using REACT_IMPL::DomainSpecificInputManager;
        DomainSpecificInputManager<D>::Instance()
            .AsyncTransaction(0, nullptr, std::forward<F>(func));
    }

    template <typename F>
    static void AsyncTransaction(TurnFlagsT flags, F&& func)
    {
        using REACT_IMPL::DomainSpecificInputManager;
        DomainSpecificInputManager<D>::Instance()
            .AsyncTransaction(flags, nullptr, std::forward<F>(func));
    }

    template <typename F>
    static void AsyncTransaction(TransactionStatus& status, F&& func)
    {
        using REACT_IMPL::DomainSpecificInputManager;

        DomainSpecificInputManager<D>::Instance()
            .AsyncTransaction(0, status.state_, std::forward<F>(func));
    }

    template <typename F>
    static void AsyncTransaction(TurnFlagsT flags, TransactionStatus& status, F&& func)
    {
        using REACT_IMPL::DomainSpecificInputManager;
        DomainSpecificInputManager<D>::Instance()
            .AsyncTransaction(flags, status.state_, std::forward<F>(func));
    }

#ifdef REACT_ENABLE_LOGGING
    ///////////////////////////////////////////////////////////////////////////////////////////////
    /// Log
    ///////////////////////////////////////////////////////////////////////////////////////////////
    static EventLog& Log()
    {
        static EventLog instance;
        return instance;
    }
#endif //REACT_ENABLE_LOGGING
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Continuation
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename TSourceDomain,
    typename TTargetDomain
>
class Continuation
{
    using NodePtrT  = REACT_IMPL::NodeBasePtrT<TSourceDomain>;

public:
    using SourceDomainT = TSourceDomain;
    using TargetDomainT = TTargetDomain;

    Continuation() = default;
    Continuation(const Continuation&) = delete;
    Continuation& operator=(const Continuation&) = delete;

    Continuation(Continuation&& other) :
        nodePtr_( std::move(other.nodePtr_) )
    {}

    explicit Continuation(NodePtrT&& nodePtr) :
        nodePtr_( std::move(nodePtr) )
    {}    

    Continuation& operator=(Continuation&& other)
    {
        nodePtr_ = std::move(other.nodePtr_);
        return *this;
    }

private:
    NodePtrT    nodePtr_;
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
auto MakeContinuation(const Signal<D,S>& trigger, FIn&& func)
    -> Continuation<D,DOut>
{
    using REACT_IMPL::SignalContinuationNode;
    using F = typename std::decay<FIn>::type;

    return Continuation<D,DOut>(
        std::make_shared<SignalContinuationNode<D,DOut,S,F>>(
            trigger.NodePtr(), std::forward<FIn>(func)));
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
auto MakeContinuation(const Events<D,E>& trigger, FIn&& func)
    -> Continuation<D,DOut>
{
    using REACT_IMPL::EventContinuationNode;
    using F = typename std::decay<FIn>::type;

    return Continuation<D,DOut>(
        std::make_shared<EventContinuationNode<D,DOut,E,F>>(
            trigger.NodePtr(), std::forward<FIn>(func)));
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
auto MakeContinuation(const Events<D,E>& trigger,
                      const SignalPack<D,TDepValues...>& depPack, FIn&& func)
    -> Continuation<D,DOut>
{
    using REACT_IMPL::SyncedContinuationNode;
    using F = typename std::decay<FIn>::type;

    struct NodeBuilder_
    {
        NodeBuilder_(const Events<D,E>& trigger, FIn&& func) :
            MyTrigger( trigger ),
            MyFunc( std::forward<FIn>(func) )
        {}

        auto operator()(const Signal<D,TDepValues>& ... deps)
            -> Continuation<D,DOut>
        {
            return Continuation<D,DOut>(
                std::make_shared<SyncedContinuationNode<D,DOut,E,F,TDepValues...>>(
                    MyTrigger.NodePtr(),
                    std::forward<FIn>(MyFunc), deps.NodePtr() ...));
        }

        const Events<D,E>&  MyTrigger;
        FIn                 MyFunc;
    };

    return REACT_IMPL::apply(
        NodeBuilder_( trigger, std::forward<FIn>(func) ),
        depPack.Data);
}

/******************************************/ REACT_END /******************************************/

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Policy
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename TEngine = ToposortEngine<sequential>
>
struct DomainPolicy
{
    using Engine = TEngine;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Ensure singletons are created immediately after domain declaration (TODO hax)
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class DomainInitializer
{
public:
    DomainInitializer()
    {
#ifdef REACT_ENABLE_LOGGING
        D::Log();
#endif //REACT_ENABLE_LOGGING

        D::Engine::Instance();
        DomainSpecificObserverRegistry<D>::Instance();
        DomainSpecificInputManager<D>::Instance();
    }
};

/****************************************/ REACT_IMPL_END /***************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Domain definition macro
///////////////////////////////////////////////////////////////////////////////////////////////////
#define REACTIVE_DOMAIN(name, ...) \
    struct name : public REACT::DomainBase<name, REACT_IMPL::DomainPolicy<__VA_ARGS__ >> {}; \
    REACT_IMPL::DomainInitializer< name > name ## _initializer_;

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