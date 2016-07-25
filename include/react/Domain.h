
//          Copyright Sebastian Jeckel 2016.
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
template <typename T>
class Signal;

template <typename T>
class VarSignal;

template <typename T>
class Events;

template <typename T>
class EventSource;

enum class Token;

class Observer;

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
    TransactionStatus(TransactionStatus&& other) :
        statePtr_( std::move(other.statePtr_) )
    {
        other.statePtr_ = StateT::Create();
    }

    // Move assignment
    TransactionStatus& operator=(TransactionStatus&& other)
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