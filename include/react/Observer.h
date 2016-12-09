
//          Copyright Sebastian Jeckel 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_OBSERVER_H_INCLUDED
#define REACT_OBSERVER_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"
#include "react/API.h"
#include "react/Group.h"

#include <memory>
#include <utility>

#include "react/detail/graph/ObserverNodes.h"

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ObserverBase
///////////////////////////////////////////////////////////////////////////////////////////////////
class ObserverBase
{
private:
    using NodeType = REACT_IMPL::ObserverNode;

public:
    // Private node ctor
    explicit ObserverBase(std::shared_ptr<NodeType>&& nodePtr) :
        nodePtr_( std::move(nodePtr) )
        { }

    void Cancel()
        { nodePtr_.reset(); }

    bool IsCancelled() const
        { return nodePtr_ != nullptr; }

protected:
    ObserverBase() = default;

    ObserverBase(const ObserverBase&) = default;
    ObserverBase& operator=(const ObserverBase&) = default;

    ObserverBase(ObserverBase&&) = default;
    ObserverBase& operator=(ObserverBase&&) = default;

    auto NodePtr() -> std::shared_ptr<NodeType>&
        { return nodePtr_; }

    auto NodePtr() const -> const std::shared_ptr<NodeType>&
        { return nodePtr_; }

    template <typename F, typename T1, typename ... Ts>
    auto CreateSignalObserverNode(const std::shared_ptr<REACT_IMPL::ReactiveGraph>& graphPtr, F&& func, const SignalBase<T1>& dep1, const SignalBase<Ts>& ... deps) -> decltype(auto)
    {
				using REACT_IMPL::PrivateSignalLinkNodeInterface;
        using ObsNodeType = REACT_IMPL::SignalObserverNode<typename std::decay<F>::type, T1, Ts ...>;

        return std::make_shared<ObsNodeType>(
            graphPtr,
            std::forward<F>(func),
            PrivateSignalLinkNodeInterface::GetLocalSignalNodePtr(graphPtr, dep1), PrivateSignalLinkNodeInterface::GetLocalSignalNodePtr(graphPtr, deps) ...);
    }

    template <typename F, typename T1, typename ... Ts>
    auto CreateSignalObserverNode(F&& func, const SignalBase<T1>& dep1, const SignalBase<Ts>& ... deps) -> decltype(auto)
    {
        using REACT_IMPL::PrivateNodeInterface;
        return CreateSignalObserverNode(PrivateNodeInterface::GraphPtr(dep1), std::forward<F>(func), dep1, deps ...);
    }

    template <typename F, typename T>
    auto CreateEventObserverNode(const std::shared_ptr<REACT_IMPL::ReactiveGraph>& graphPtr, F&& func, const EventBase<T>& dep) -> decltype(auto)
    {
				using REACT_IMPL::PrivateEventLinkNodeInterface;
        using ObsNodeType = REACT_IMPL::EventObserverNode<typename std::decay<F>::type, T>;

        return std::make_shared<ObsNodeType>(graphPtr, std::forward<F>(func), PrivateEventLinkNodeInterface::GetLocalEventNodePtr(graphPtr, dep));
    }

    template <typename F, typename T>
    auto CreateEventObserverNode(F&& func, const EventBase<T>& dep) -> decltype(auto)
    {
        using REACT_IMPL::PrivateNodeInterface;
        return CreateSignalObserverNode(PrivateNodeInterface::GraphPtr(dep1), std::forward<F>(func), dep1, deps ...);
    }

    template <typename F, typename T, typename ... Us>
    auto CreateSyncedEventObserverNode(F&& func, const EventBase<T>& dep, const SignalBase<Us>& ... syncs) -> decltype(auto)
    {
        using REACT_IMPL::PrivateNodeInterface;
				using REACT_IMPL::PrivateSignalLinkNodeInterface;
        using ObsNodeType = REACT_IMPL::SyncedEventObserverNode<typename std::decay<F>::type, T, Us ...>;

				const auto& graphPtr = PrivateNodeInterface::GraphPtr(dep);

        return std::make_shared<ObsNodeType>(
            graphPtr, std::forward<F>(func), PrivateSignalLinkNodeInterface::GetLocalSignalNodePtr(graphPtr, dep), PrivateSignalLinkNodeInterface::GetLocalSignalNodePtr(graphPtr, syncs) ...);
    }

private:
    std::shared_ptr<NodeType> nodePtr_;

    friend struct REACT_IMPL::PrivateNodeInterface;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Observer
///////////////////////////////////////////////////////////////////////////////////////////////////
template <>
class Observer<unique> : public ObserverBase
{
public:
    using ObserverBase::ObserverBase;

    Observer() = delete;

    Observer(const Observer&) = delete;
    Observer& operator=(const Observer&) = delete;

    Observer(Observer&&) = default;
    Observer& operator=(Observer&&) = default;

    // Construct signal observer with implicit group
    template <typename F, typename ... Ts>
    Observer(F&& func, const SignalBase<Ts>& ... subjects) :
        Observer::ObserverBase( CreateSignalObserverNode(std::forward<F>(func), subjects ...) )
        { }

    // Construct signal observer with explicit group
    template <typename F, typename ... Ts>
    Observer(const ReactiveGroupBase& group, F&& func, const SignalBase<Ts>& ... subjects) :
        Observer::ObserverBase( CreateSignalObserverNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group), std::forward<F>(func), subjects ...) )
        { }

    // Construct event observer with implicit group
    template <typename F, typename T>
    Observer(F&& func, const EventBase<T>& subject) :
        Observer::ObserverBase( CreateEventObserverNode(std::forward<F>(func), subject) )
        { }

    // Construct event observer with explicit group
    template <typename F, typename T>
    Observer(const ReactiveGroupBase& group, F&& func, const EventBase<T>& subject) :
        Observer::ObserverBase( CreateEventObserverNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group), std::forward<F>(func), subject) )
        { }

    // Constructed synced event observer with implicit group
    template <typename F, typename T, typename ... Us>
    Observer(F&& func, const EventBase<T>& subject, const SignalBase<Us>& ... signals) :
        Observer::ObserverBase( CreateSyncedEventObserverNode(std::forward<F>(func), subject, signals ...) )
        { }

    // Constructed synced event observer with explicit group
    template <typename F, typename T, typename ... Us>
    Observer(const ReactiveGroupBase& group, F&& func, const EventBase<T>& subject, const SignalBase<Us>& ... signals) :
        Observer::ObserverBase( CreateSyncedEventObserverNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group), std::forward<F>(func), subject, signals ...) )
        { }
};

template <>
class Observer<shared> : public ObserverBase
{
public:
    using ObserverBase::ObserverBase;

    Observer() = delete;

    Observer(const Observer&) = default;
    Observer& operator=(const Observer&) = default;

    Observer(Observer&&) = default;
    Observer& operator=(Observer&&) = default;

    // Construct from unique
    Observer(Observer<unique>&& other) :
        Observer::ObserverBase( std::move(other) )
        { }

    // Assign from unique
    Observer& operator=(Observer<unique>&& other)
        { Observer::ObserverBase::operator=(std::move(other)); return *this; }

    // Construct signal observer with implicit group
    template <typename F, typename ... Ts>
    Observer(F&& func, const SignalBase<Ts>& ... subjects) :
        Observer::ObserverBase( CreateSignalObserverNode(std::forward<F>(func), subjects ...) )
        { }

    // Construct signal observer with explicit group
    template <typename F, typename ... Ts>
    Observer(const ReactiveGroupBase& group, F&& func, const SignalBase<Ts>& ... subjects) :
        Observer::ObserverBase( CreateSignalObserverNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group), std::forward<F>(func), subjects ...) )
        { }

    // Construct event observer with implicit group
    template <typename F, typename T>
    Observer(F&& func, const EventBase<T>& subject) :
        Observer::ObserverBase( CreateEventObserverNode(std::forward<F>(func), subject) )
        { }

    // Construct event observer with explicit group
    template <typename F, typename T>
    Observer(const ReactiveGroupBase& group, F&& func, const EventBase<T>& subject) :
        Observer::ObserverBase( CreateEventObserverNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group), std::forward<F>(func), subject) )
        { }

    // Constructed synced event observer with implicit group
    template <typename F, typename T, typename ... Us>
    Observer(F&& func, const EventBase<T>& subject, const SignalBase<Us>& ... signals) :
        Observer::ObserverBase( CreateSyncedEventObserverNode(std::forward<F>(func), subject, signals ...) )
        { }

    // Constructed synced event observer with explicit group
    template <typename F, typename T, typename ... Us>
    Observer(const ReactiveGroupBase& group, F&& func, const EventBase<T>& subject, const SignalBase<Us>& ... signals) :
        Observer::ObserverBase( CreateSyncedEventObserverNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group), std::forward<F>(func), subject, signals ...) )
        { }
};

/******************************************/ REACT_END /******************************************/

#endif // REACT_OBSERVER_H_INCLUDED