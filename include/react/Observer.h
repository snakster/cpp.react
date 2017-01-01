
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
    Observer(const Observer&) = default;
    Observer& operator=(const Observer&) = default;

    Observer(Observer&&) = default;
    Observer& operator=(Observer&&) = default;

    // Construct signal observer with explicit group
    template <typename F, typename T1, typename ... Ts>
    Observer(const Group& group, F&& func, const Signal<T1>& subject1, const Signal<Ts>& ... subjects) :
        Observer::Observer(REACT_IMPL::CtorTag{ }, CreateSignalObserverNode(group, std::forward<F>(func), subject1, subjects ...))
        { }

    // Construct signal observer with implicit group
    template <typename F, typename T1, typename ... Ts>
    Observer(F&& func, const Signal<T1>& subject1, const Signal<Ts>& ... subjects) :
        Observer::Observer(REACT_IMPL::CtorTag{ }, CreateSignalObserverNode(subject1.GetGroup(), std::forward<F>(func), subject1, subjects ...))
        { }

    // Construct event observer with explicit group
    template <typename F, typename T>
    Observer(const Group& group, F&& func, const Event<T>& subject) :
        Observer::Observer(REACT_IMPL::CtorTag{ }, CreateEventObserverNode(group, std::forward<F>(func), subject))
        { }

    // Construct event observer with implicit group
    template <typename F, typename T>
    Observer(F&& func, const Event<T>& subject) :
        Observer::Observer(REACT_IMPL::CtorTag{ }, CreateEventObserverNode(subject.GetGroup(), std::forward<F>(func), subject))
        { }

    // Constructed synced event observer with explicit group
    template <typename F, typename T, typename ... Us>
    Observer(const Group& group, F&& func, const Event<T>& subject, const Signal<Us>& ... signals) :
        Observer::Observer(REACT_IMPL::CtorTag{ }, CreateSyncedEventObserverNode(group, std::forward<F>(func), subject, signals ...))
        { }

    // Constructed synced event observer with implicit group
    template <typename F, typename T, typename ... Us>
    Observer(F&& func, const Event<T>& subject, const Signal<Us>& ... signals) :
        Observer::Observer(REACT_IMPL::CtorTag{ }, CreateSyncedEventObserverNode(subject.GetGroup(), std::forward<F>(func), subject, signals ...))
        { }

public: //Internal
    // Private node ctor
    Observer(REACT_IMPL::CtorTag, std::shared_ptr<NodeType>&& nodePtr) :
        nodePtr_(std::move(nodePtr))
        { }

    auto NodePtr() -> std::shared_ptr<NodeType>&
        { return nodePtr_; }

    auto NodePtr() const -> const std::shared_ptr<NodeType>&
        { return nodePtr_; }

protected:
    template <typename F, typename T1, typename ... Ts>
    auto CreateSignalObserverNode(const Group& group, F&& func, const Signal<T1>& dep1, const Signal<Ts>& ... deps) -> decltype(auto)
    {
        using REACT_IMPL::SignalObserverNode;
        return std::make_shared<SignalObserverNode<typename std::decay<F>::type, T1, Ts ...>>(
            group, std::forward<F>(func), SameGroupOrLink(group, dep1), SameGroupOrLink(group, deps) ...);
    }

    template <typename F, typename T>
    auto CreateEventObserverNode(const Group& group, F&& func, const Event<T>& dep) -> decltype(auto)
    {
        using REACT_IMPL::EventObserverNode;
        return std::make_shared<EventObserverNode<typename std::decay<F>::type, T>>(
            group, std::forward<F>(func), SameGroupOrLink(group, dep));
    }

    template <typename F, typename T, typename ... Us>
    auto CreateSyncedEventObserverNode(const Group& group, F&& func, const Event<T>& dep, const Signal<Us>& ... syncs) -> decltype(auto)
    {
        using REACT_IMPL::SyncedEventObserverNode;
        return std::make_shared<SyncedEventObserverNode<typename std::decay<F>::type, T, Us ...>>(
            group, std::forward<F>(func), SameGroupOrLink(group, dep), SameGroupOrLink(group, syncs) ...);
    }

private:
    std::shared_ptr<NodeType> nodePtr_;
};

/******************************************/ REACT_END /******************************************/

#endif // REACT_OBSERVER_H_INCLUDED