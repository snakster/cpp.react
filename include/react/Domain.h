
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

// Include all engines for convenience
#include "react/engine/PulsecountEngine.h"
#include "react/engine/SubtreeEngine.h"
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

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Common types & constants
///////////////////////////////////////////////////////////////////////////////////////////////////
using REACT_IMPL::TurnFlagsT;

// ETurnFlags
using REACT_IMPL::ETurnFlags;
using REACT_IMPL::allow_merging;

#ifdef REACT_ENABLE_LOGGING
    using REACT_IMPL::EventLog;
#endif //REACT_ENABLE_LOGGING

// Domain modes
enum EDomainMode
{
    sequential,
    sequential_concurrent,
    parallel,
    parallel_concurrent
};

// Expose enum types so aliases for engines can be declared, but don't
// expose the actual enum values as they are reserved for internal use.
using REACT_IMPL::EInputMode;
using REACT_IMPL::EPropagationMode;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// TransactionStatus
///////////////////////////////////////////////////////////////////////////////////////////////////
class TransactionStatus
{
    using StateT = REACT_IMPL::SharedWaitingState;
    using PtrT = REACT_IMPL::WaitingStatePtrT;

public:
    TransactionStatus() :
        statePtr_( StateT::Create() )
    {}

    TransactionStatus(const TransactionStatus&) = default;

    TransactionStatus(TransactionStatus&& other) :
        statePtr_( std::move(other.statePtr_) )
    {}

    TransactionStatus& operator=(const TransactionStatus&) = default;

    TransactionStatus& operator=(TransactionStatus&& other)
    {
        statePtr_ = std::move(other.statePtr_);
        return *this;
    }

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
    friend void AsyncTransaction(TurnFlagsT flags, TransactionStatus& status, F&& func);
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
        Policy::input_mode == REACT_IMPL::concurrent_input;

    static const bool is_parallel =
        Policy::propagation_mode == REACT_IMPL::parallel_propagation;

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
auto MakeContinuation(TurnFlagsT flags, const Signal<D,S>& trigger, FIn&& func)
    -> Continuation<D,DOut>
{
    static_assert(DOut::is_concurrent,
        "MakeContinuation requires support for concurrent input to target domain.");

    using REACT_IMPL::SignalContinuationNode;
    using F = typename std::decay<FIn>::type;

    return Continuation<D,DOut>(
        std::make_shared<SignalContinuationNode<D,DOut,S,F>>(
            flags, trigger.NodePtr(), std::forward<FIn>(func)));
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
auto MakeContinuation(TurnFlagsT flags, const Events<D,E>& trigger, FIn&& func)
    -> Continuation<D,DOut>
{
    static_assert(DOut::is_concurrent,
        "MakeContinuation requires support for concurrent input to target domain.");

    using REACT_IMPL::EventContinuationNode;
    using F = typename std::decay<FIn>::type;

    return Continuation<D,DOut>(
        std::make_shared<EventContinuationNode<D,DOut,E,F>>(
            flags, trigger.NodePtr(), std::forward<FIn>(func)));
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
auto MakeContinuation(TurnFlagsT flags, const Events<D,E>& trigger,
                      const SignalPack<D,TDepValues...>& depPack, FIn&& func)
    -> Continuation<D,DOut>
{
    static_assert(DOut::is_concurrent,
        "MakeContinuation requires support for concurrent input to target domain.");

    using REACT_IMPL::SyncedContinuationNode;
    using F = typename std::decay<FIn>::type;

    struct NodeBuilder_
    {
        NodeBuilder_(TurnFlagsT flags, const Events<D,E>& trigger, FIn&& func) :
            MyFlags( flags ),
            MyTrigger( trigger ),
            MyFunc( std::forward<FIn>(func) )
        {}

        auto operator()(const Signal<D,TDepValues>& ... deps)
            -> Continuation<D,DOut>
        {
            return Continuation<D,DOut>(
                std::make_shared<SyncedContinuationNode<D,DOut,E,F,TDepValues...>>(
                    MyFlags,
                    MyTrigger.NodePtr(),
                    std::forward<FIn>(MyFunc), deps.NodePtr() ...));
        }

        TurnFlagsT          MyFlags;
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
void DoTransaction(TurnFlagsT flags, F&& func)
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
    static_assert(D::is_concurrent, "AsyncTransaction requires concurrent domain.");

    using REACT_IMPL::DomainSpecificInputManager;
    DomainSpecificInputManager<D>::Instance()
        .AsyncTransaction(0, nullptr, std::forward<F>(func));
}

template <typename D, typename F>
void AsyncTransaction(TurnFlagsT flags, F&& func)
{
    static_assert(D::is_concurrent, "AsyncTransaction requires concurrent domain.");

    using REACT_IMPL::DomainSpecificInputManager;
    DomainSpecificInputManager<D>::Instance()
        .AsyncTransaction(flags, nullptr, std::forward<F>(func));
}

template <typename D, typename F>
void AsyncTransaction(TransactionStatus& status, F&& func)
{
    static_assert(D::is_concurrent, "AsyncTransaction requires concurrent domain.");

    using REACT_IMPL::DomainSpecificInputManager;

    DomainSpecificInputManager<D>::Instance()
        .AsyncTransaction(0, status.statePtr_, std::forward<F>(func));
}

template <typename D, typename F>
void AsyncTransaction(TurnFlagsT flags, TransactionStatus& status, F&& func)
{
    static_assert(D::is_concurrent, "AsyncTransaction requires concurrent domain.");

    using REACT_IMPL::DomainSpecificInputManager;
    DomainSpecificInputManager<D>::Instance()
        .AsyncTransaction(flags, status.statePtr_, std::forward<F>(func));
}

/******************************************/ REACT_END /******************************************/

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ModeSelector - Translate domain mode to individual propagation and input modes
///////////////////////////////////////////////////////////////////////////////////////////////////
template <EDomainMode>
struct ModeSelector;

template <>
struct ModeSelector<sequential>
{
    static const EInputMode         input       = consecutive_input;
    static const EPropagationMode   propagation = sequential_propagation;
};

template <>
struct ModeSelector<sequential_concurrent>
{
    static const EInputMode         input       = concurrent_input;
    static const EPropagationMode   propagation = sequential_propagation;
};

template <>
struct ModeSelector<parallel>
{
    static const EInputMode         input       = consecutive_input;
    static const EPropagationMode   propagation = parallel_propagation;
};

template <>
struct ModeSelector<parallel_concurrent>
{
    static const EInputMode         input       = concurrent_input;
    static const EPropagationMode   propagation = parallel_propagation;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// GetDefaultEngine - Get default engine type for given propagation mode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <EPropagationMode>
struct GetDefaultEngine;

template <>
struct GetDefaultEngine<sequential_propagation>
{
    using Type = ToposortEngine<sequential_propagation>;
};

template <>
struct GetDefaultEngine<parallel_propagation>
{
    using Type = SubtreeEngine<parallel_propagation>;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EngineTypeBuilder - Instantiate the given template engine type with mode.
///////////////////////////////////////////////////////////////////////////////////////////////////
template <EPropagationMode>
struct DefaultEnginePlaceholder;

// Concrete engine template type
template
<
    EPropagationMode mode,
    template <EPropagationMode> class TTEngine
>
struct EngineTypeBuilder
{
    using Type = TTEngine<mode>;
};

// Placeholder engine type - use default engine for given mode
template
<
    EPropagationMode mode
>
struct EngineTypeBuilder<mode,DefaultEnginePlaceholder>
{
    using Type = typename GetDefaultEngine<mode>::Type;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// DomainPolicy
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    EDomainMode mode,
    template <EPropagationMode> class TTEngine = DefaultEnginePlaceholder
>
struct DomainPolicy
{
    static const EInputMode         input_mode          = ModeSelector<mode>::input;
    static const EPropagationMode   propagation_mode    = ModeSelector<mode>::propagation;

    using Engine = typename EngineTypeBuilder<propagation_mode,TTEngine>::Type;
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
#define REACTIVE_DOMAIN(name, ...)                                                          \
    struct name :                                                                           \
        public REACT::DomainBase<name, REACT_IMPL::DomainPolicy< __VA_ARGS__ >> {};         \
    REACT_IMPL::DomainInitializer<name> name ## _initializer_;

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