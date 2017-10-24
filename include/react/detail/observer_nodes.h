
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_OBSERVER_NODES_H_INCLUDED
#define REACT_DETAIL_OBSERVER_NODES_H_INCLUDED

#pragma once

#include "react/detail/defs.h"
#include "react/api.h"

#include <memory>
#include <utility>

#include "node_base.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward declarations
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class SignalNode;

template <typename E>
class EventStreamNode;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SignalObserverNode
///////////////////////////////////////////////////////////////////////////////////////////////////
class ObserverNode : public NodeBase
{
public:
    explicit ObserverNode(const Group& group) :
        ObserverNode::NodeBase( group )
    { }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SignalObserverNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename F, typename ... TDeps>
class SignalObserverNode : public ObserverNode
{
public:
    template <typename FIn>
    SignalObserverNode(const Group& group, FIn&& func, const Signal<TDeps>& ... deps) :
        SignalObserverNode::ObserverNode( group ),
        func_( std::forward<FIn>(func) ),
        depHolder_( deps ... )
    {
        this->RegisterMe(NodeCategory::output);
        REACT_EXPAND_PACK(this->AttachToMe(GetInternals(deps).GetNodeId()));
    }

    ~SignalObserverNode()
    {
        std::apply([this] (const auto& ... deps)
            { REACT_EXPAND_PACK(this->DetachFromMe(GetInternals(deps).GetNodeId())); }, depHolder_);
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        std::apply([this] (const auto& ... deps) { this->func_(GetInternals(deps).Value() ...); }, depHolder_);
        return UpdateResult::unchanged;
    }

private:
    F func_;

    std::tuple<Signal<TDeps> ...> depHolder_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventObserverNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename F, typename E>
class EventObserverNode : public ObserverNode
{
public:
    template <typename FIn>
    EventObserverNode(const Group& group, FIn&& func, const Event<E>& subject) :
        EventObserverNode::ObserverNode( group ),
        func_( std::forward<FIn>(func) ),
        subject_( subject )
    {
        this->RegisterMe(NodeCategory::output);
        this->AttachToMe(GetInternals(subject).GetNodeId());
    }

    ~EventObserverNode()
    {
        this->DetachFromMe(GetInternals(subject_).GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        func_(GetInternals(subject_).Events());
        return UpdateResult::unchanged;
    }

private:
    F func_;

    Event<E> subject_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncedEventObserverNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename F, typename E, typename ... TSyncs>
class SyncedEventObserverNode : public ObserverNode
{
public:
    template <typename FIn>
    SyncedEventObserverNode(const Group& group, FIn&& func, const Event<E>& subject, const Signal<TSyncs>& ... syncs) :
        SyncedEventObserverNode::ObserverNode( group ),
        func_( std::forward<FIn>(func) ),
        subject_( subject ),
        syncHolder_( syncs ... )
    {
        this->RegisterMe(NodeCategory::output);
        this->AttachToMe(GetInternals(subject).GetNodeId());
        REACT_EXPAND_PACK(this->AttachToMe(GetInternals(syncs).GetNodeId()));
    }

    ~SyncedEventObserverNode()
    {
        std::apply([this] (const auto& ... syncs)
            { REACT_EXPAND_PACK(this->DetachFromMe(GetInternals(syncs).GetNodeId())); }, syncHolder_);
        this->DetachFromMe(GetInternals(subject_).GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        // Updates might be triggered even if only sync nodes changed. Ignore those.
        if (GetInternals(this->subject_).Events().empty())
            return UpdateResult::unchanged;

        std::apply([this] (const auto& ... syncs)
            { func_(EventRange<E>( GetInternals(this->subject_).Events() ), GetInternals(syncs).Value() ...); }, syncHolder_);

        return UpdateResult::unchanged;
    }

private:
    F func_;

    Event<E> subject_;

    std::tuple<Signal<TSyncs> ...> syncHolder_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_OBSERVER_NODES_H_INCLUDED