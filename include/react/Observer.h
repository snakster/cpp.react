
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_OBSERVER_H_INCLUDED
#define REACT_OBSERVER_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <memory>
#include <utility>

#include "react/common/Util.h"
#include "react/detail/IReactiveNode.h"
#include "react/detail/ObserverBase.h"
#include "react/detail/graph/ObserverNodes.h"

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward declarations
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename S>
class Signal;

template <typename D, typename ... TValues>
class SignalPack;

template <typename D, typename E>
class Events;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Observer
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class Observer
{
private:
    using SubjectT  = REACT_IMPL::NodeBasePtrT<D>;
    using NodeT     = REACT_IMPL::IObserver;

public:
    Observer() :
        nodePtr_( nullptr )
    {}

    Observer(const Observer&) = delete;
    Observer& operator=(const Observer&) = delete;

    Observer(Observer&& other) :
        nodePtr_( other.nodePtr_ ),
        subject_( std::move(other.subject_) )
    {
        other.nodePtr_ = nullptr;
    }

    Observer& operator=(Observer&& other)
    {
        nodePtr_ = other.nodePtr_;
        subject_ = std::move(other.subject_);

        other.nodePtr_ = nullptr;

        return *this;
    }

    Observer(NodeT* nodePtr, const SubjectT& subject) :
        nodePtr_( nodePtr ),
        subject_( subject )
    {}

    bool IsValid() const
    {
        return nodePtr_ != nullptr;
    }

    void Detach()
    {
        REACT_ASSERT(IsValid(), "Detach on invalid Observer.");
        REACT_IMPL::DomainSpecificObserverRegistry<D>::Instance().Unregister(nodePtr_);
    }

private:
    // Ownership managed by registry
    NodeT*      nodePtr_;

    // While the observer handle exists, the subject is not destroyed
    SubjectT    subject_;    
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ScopedObserver
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class ScopedObserver
{
public:
    ScopedObserver() = delete;
    ScopedObserver(const ScopedObserver&) = delete;
    ScopedObserver& operator=(const ScopedObserver&) = delete;

    ScopedObserver(ScopedObserver&& other) :
        obs_( std::move(other.obs_) )
    {}

    ScopedObserver& operator=(ScopedObserver&& other)
    {
        obs_ = std::move(other.obs_);
    }

    ScopedObserver(Observer<D>&& obs) :
        obs_( std::move(obs) )
    {}

    ~ScopedObserver()
    {
        if (obs_.IsValid())
            obs_.Detach();
    }

private:
    Observer<D>     obs_;    
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Observe - Signals
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename FIn,
    typename S
>
auto Observe(const Signal<D,S>& subject, FIn&& func)
    -> Observer<D>
{
    using REACT_IMPL::IObserver;
    using REACT_IMPL::SignalObserverNode;
    using REACT_IMPL::DomainSpecificObserverRegistry;

    using F = typename std::decay<FIn>::type;

    std::unique_ptr<IObserver> obsPtr{
        new SignalObserverNode<D,S,F>{
            subject.NodePtr(), std::forward<FIn>(func)}};

    auto* rawObsPtr = DomainSpecificObserverRegistry<D>::Instance()
        .Register(std::move(obsPtr), subject.NodePtr().get());

    return Observer<D>{ rawObsPtr, subject.NodePtr() };
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Observe - Events
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename FIn,
    typename E
>
auto Observe(const Events<D,E>& subject, FIn&& func)
    -> Observer<D>
{
    using REACT_IMPL::IObserver;
    using REACT_IMPL::EventObserverNode;
    using REACT_IMPL::DomainSpecificObserverRegistry;

    using F = typename std::decay<FIn>::type;

    std::unique_ptr<IObserver> obsPtr{
        new EventObserverNode<D,E,F>{
            subject.NodePtr(), std::forward<FIn>(func)}};

    auto* rawObsPtr = DomainSpecificObserverRegistry<D>::Instance()
        .Register(std::move(obsPtr), subject.NodePtr().get());

    return Observer<D>(rawObsPtr, subject.NodePtr());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Observe - Synced
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename FIn,
    typename E,
    typename ... TDepValues
>
auto Observe(const Events<D,E>& subject,
             const SignalPack<D,TDepValues...>& depPack, FIn&& func)
    -> Observer<D>
{
    using REACT_IMPL::IObserver;
    using REACT_IMPL::SyncedObserverNode;
    using REACT_IMPL::DomainSpecificObserverRegistry;

    using F = typename std::decay<FIn>::type;

    struct NodeBuilder_
    {
        NodeBuilder_(const Events<D,E>& subject, FIn&& func) :
            MySubject( subject ),
            MyFunc( std::forward<FIn>(func) )
        {}

        auto operator()(const Signal<D,TDepValues>& ... deps)
            -> std::unique_ptr<IObserver>
        {
            return std::unique_ptr<IObserver> {
                new SyncedObserverNode<D,E,F,TDepValues ...> {
                    MySubject.NodePtr(), std::forward<FIn>(MyFunc), deps.NodePtr() ...}};
        }

        const Events<D,E>& MySubject;
        FIn MyFunc;
    };

    auto obsPtr = REACT_IMPL::apply(
        NodeBuilder_( subject, std::forward<FIn>(func) ),
        depPack.Data);

    auto* rawObsPtr = DomainSpecificObserverRegistry<D>::Instance()
        .Register(std::move(obsPtr), subject.NodePtr().get());

    return Observer<D>(rawObsPtr, subject.NodePtr());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// DetachAllObservers
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TSubject>
void DetachAllObservers(const TSubject& subject)
{
    using D = typename TSubject::DomainT;

    REACT_IMPL::DomainSpecificObserverRegistry<D>::Instance().UnregisterFrom(
        subject.NodePtr().get());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// DetachThisObserver
///////////////////////////////////////////////////////////////////////////////////////////////////
inline void DetachThisObserver()
{
    REACT_IMPL::ThreadLocalObserverState<>::ShouldDetach = true;
}

/******************************************/ REACT_END /******************************************/

#endif // REACT_OBSERVER_H_INCLUDED