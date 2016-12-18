
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
class Observer
{
private:
    using NodeType = REACT_IMPL::ObserverNode;

public:
    Observer() = default;

    Observer(const Observer&) = default;
    Observer& operator=(const Observer&) = default;

    Observer(Observer&&) = default;
    Observer& operator=(Observer&&) = default;

    // Construct signal observer with implicit group
    template <typename F, typename ... Ts>
    Observer(F&& func, const Signal<Ts>& ... subjects) :
        Observer::Observer(CreateSignalObserverNode(std::forward<F>(func), subjects ...))
    { }

    // Construct signal observer with explicit group
    template <typename F, typename ... Ts>
    Observer(const ReactiveGroup& group, F&& func, const Signal<Ts>& ... subjects) :
        Observer::Observer(CreateSignalObserverNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group), std::forward<F>(func), subjects ...))
    { }

    // Construct event observer with implicit group
    template <typename F, typename T>
    Observer(F&& func, const Event<T>& subject) :
        Observer::Observer(CreateEventObserverNode(std::forward<F>(func), subject))
    { }

    // Construct event observer with explicit group
    template <typename F, typename T>
    Observer(const ReactiveGroup& group, F&& func, const Event<T>& subject) :
        Observer::Observer(CreateEventObserverNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group), std::forward<F>(func), subject))
    { }

    // Constructed synced event observer with implicit group
    template <typename F, typename T, typename ... Us>
    Observer(F&& func, const Event<T>& subject, const Signal<Us>& ... signals) :
        Observer::Observer(CreateSyncedEventObserverNode(std::forward<F>(func), subject, signals ...))
    { }

    // Constructed synced event observer with explicit group
    template <typename F, typename T, typename ... Us>
    Observer(const ReactiveGroup& group, F&& func, const Event<T>& subject, const Signal<Us>& ... signals) :
        Observer::Observer(CreateSyncedEventObserverNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group), std::forward<F>(func), subject, signals ...))
    { }

    void Cancel()
        { nodePtr_.reset(); }

    bool IsCancelled() const
        { return nodePtr_ != nullptr; }

protected:
    // Private node ctor
    explicit Observer(std::shared_ptr<NodeType>&& nodePtr) :
        nodePtr_(std::move(nodePtr))
        { }

    auto NodePtr() -> std::shared_ptr<NodeType>&
        { return nodePtr_; }

    auto NodePtr() const -> const std::shared_ptr<NodeType>&
        { return nodePtr_; }

    template <typename F, typename T1, typename ... Ts>
    auto CreateSignalObserverNode(const std::shared_ptr<REACT_IMPL::ReactiveGraph>& graphPtr, F&& func, const Signal<T1>& dep1, const Signal<Ts>& ... deps) -> decltype(auto)
    {
        using REACT_IMPL::PrivateSignalLinkNodeInterface;
        using ObsNodeType = REACT_IMPL::SignalObserverNode<typename std::decay<F>::type, T1, Ts ...>;

        return std::make_shared<ObsNodeType>(
            graphPtr,
            std::forward<F>(func),
            PrivateSignalLinkNodeInterface::GetLocalNodePtr(graphPtr, dep1), PrivateSignalLinkNodeInterface::GetLocalNodePtr(graphPtr, deps) ...);
    }

    template <typename F, typename T1, typename ... Ts>
    auto CreateSignalObserverNode(F&& func, const Signal<T1>& dep1, const Signal<Ts>& ... deps) -> decltype(auto)
    {
        using REACT_IMPL::PrivateNodeInterface;
        return CreateSignalObserverNode(PrivateNodeInterface::GraphPtr(dep1), std::forward<F>(func), dep1, deps ...);
    }

    template <typename F, typename T>
    auto CreateEventObserverNode(const std::shared_ptr<REACT_IMPL::ReactiveGraph>& graphPtr, F&& func, const Event<T>& dep) -> decltype(auto)
    {
        using REACT_IMPL::PrivateEventLinkNodeInterface;
        using ObsNodeType = REACT_IMPL::EventObserverNode<typename std::decay<F>::type, T>;

        return std::make_shared<ObsNodeType>(graphPtr, std::forward<F>(func), PrivateEventLinkNodeInterface::GetLocalNodePtr(graphPtr, dep));
    }

    template <typename F, typename T>
    auto CreateEventObserverNode(F&& func, const Event<T>& dep) -> decltype(auto)
    {
        using REACT_IMPL::PrivateNodeInterface;
        return CreateEventObserverNode(PrivateNodeInterface::GraphPtr(dep), std::forward<F>(func), dep);
    }

    template <typename F, typename T, typename ... Us>
    auto CreateSyncedEventObserverNode(const std::shared_ptr<REACT_IMPL::ReactiveGraph>& graphPtr, F&& func, const Event<T>& dep, const Signal<Us>& ... syncs) -> decltype(auto)
    {
        using REACT_IMPL::PrivateEventLinkNodeInterface;
        using REACT_IMPL::PrivateSignalLinkNodeInterface;
        using ObsNodeType = REACT_IMPL::SyncedEventObserverNode<typename std::decay<F>::type, T, Us ...>;

        return std::make_shared<ObsNodeType>(
            graphPtr, std::forward<F>(func), PrivateEventLinkNodeInterface::GetLocalNodePtr(graphPtr, dep), PrivateSignalLinkNodeInterface::GetLocalNodePtr(graphPtr, syncs) ...);
    }

    template <typename F, typename T, typename ... Us>
    auto CreateSyncedEventObserverNode(F&& func, const Event<T>& dep, const Signal<Us>& ... syncs) -> decltype(auto)
    {
        using REACT_IMPL::PrivateNodeInterface;
        return CreateSyncedEventObserverNode(PrivateNodeInterface::GraphPtr(dep), std::forward<F>(func), dep, syncs ...);
    }

private:
    std::shared_ptr<NodeType> nodePtr_;

    friend struct REACT_IMPL::PrivateNodeInterface;
};

/******************************************/ REACT_END /******************************************/

#endif // REACT_OBSERVER_H_INCLUDED