
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_DOMAINBASE_H_INCLUDED
#define REACT_DETAIL_DOMAINBASE_H_INCLUDED

#pragma once

#include <memory>
#include <utility>

#include "react/detail/Defs.h"

#include "react/detail/ReactiveBase.h"
#include "react/detail/IReactiveEngine.h"
#include "react/detail/graph/ContinuationNodes.h"

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

template <typename D, typename E>
class Events;

template <typename D, typename E>
class EventSource;

enum class Token;

template <typename D>
class Observer;

template <typename D>
class ScopedObserver;

template <typename D>
class Reactor;

/******************************************/ REACT_END /******************************************/

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Common types & constants
///////////////////////////////////////////////////////////////////////////////////////////////////

// Domain modes
enum EDomainMode
{
    sequential,
    sequential_concurrent,
    parallel,
    parallel_concurrent
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
/// ContinuationBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename D2
>
class ContinuationBase : public MovableReactive<NodeBase<D>>
{
public:
    ContinuationBase() = default;

    template <typename T>
    ContinuationBase(T&& t) :
        ContinuationBase::MovableReactive( std::forward<T>(t) )
    {}
};

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
        DomainSpecificInputManager<D>::Instance();
    }
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_DOMAINBASE_H_INCLUDED