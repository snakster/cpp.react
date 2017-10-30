
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_OBSERVER_H_INCLUDED
#define REACT_OBSERVER_H_INCLUDED

#pragma once

#include "react/detail/defs.h"
#include "react/api.h"
#include "react/group.h"

#include <memory>
#include <utility>

#include "react/detail/observer_nodes.h"

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ObserverBase
///////////////////////////////////////////////////////////////////////////////////////////////////
class Observer : protected REACT_IMPL::ObserverInternals
{
private:
    using NodeType = REACT_IMPL::ObserverNode;

public:
    // Construct state observer with explicit group
    template <typename F, typename T1, typename ... Ts>
    static Observer Create(const Group& group, F&& func, const State<T1>& subject1, const State<Ts>& ... subjects)
        { return CreateStateObserverNode(group, std::forward<F>(func), subject1, subjects ...); }

    // Construct state observer with implicit group
    template <typename F, typename T1, typename ... Ts>
    static Observer Create(F&& func, const State<T1>& subject1, const State<Ts>& ... subjects)
        { return CreateStateObserverNode(subject1.GetGroup(), std::forward<F>(func), subject1, subjects ...); }

    // Construct event observer with explicit group
    template <typename F, typename T>
    static Observer Create(const Group& group, F&& func, const Event<T>& subject)
        { return  CreateEventObserverNode(group, std::forward<F>(func), subject); }

    // Construct event observer with implicit group
    template <typename F, typename T>
    static Observer Create(F&& func, const Event<T>& subject)
        { return CreateEventObserverNode(subject.GetGroup(), std::forward<F>(func), subject); }

    // Constructed synced event observer with explicit group
    template <typename F, typename T, typename ... Us>
    static Observer Create(const Group& group, F&& func, const Event<T>& subject, const State<Us>& ... states)
        { return CreateSyncedEventObserverNode(group, std::forward<F>(func), subject, states ...); }

    // Constructed synced event observer with implicit group
    template <typename F, typename T, typename ... Us>
    static Observer Create(F&& func, const Event<T>& subject, const State<Us>& ... states)
        { return CreateSyncedEventObserverNode(subject.GetGroup(), std::forward<F>(func), subject, states ...); }

    Observer(const Observer&) = default;
    Observer& operator=(const Observer&) = default;

    Observer(Observer&&) = default;
    Observer& operator=(Observer&&) = default;

protected: //Internal
    Observer(std::shared_ptr<NodeType>&& nodePtr) :
        nodePtr_(std::move(nodePtr))
    { }

private:
    template <typename F, typename T1, typename ... Ts>
    static auto CreateStateObserverNode(const Group& group, F&& func, const State<T1>& dep1, const State<Ts>& ... deps) -> decltype(auto)
    {
        using REACT_IMPL::StateObserverNode;
        return std::make_shared<StateObserverNode<typename std::decay<F>::type, T1, Ts ...>>(
            group, std::forward<F>(func), SameGroupOrLink(group, dep1), SameGroupOrLink(group, deps) ...);
    }

    template <typename F, typename T>
    static auto CreateEventObserverNode(const Group& group, F&& func, const Event<T>& dep) -> decltype(auto)
    {
        using REACT_IMPL::EventObserverNode;
        return std::make_shared<EventObserverNode<typename std::decay<F>::type, T>>(
            group, std::forward<F>(func), SameGroupOrLink(group, dep));
    }

    template <typename F, typename T, typename ... Us>
    static auto CreateSyncedEventObserverNode(const Group& group, F&& func, const Event<T>& dep, const State<Us>& ... syncs) -> decltype(auto)
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