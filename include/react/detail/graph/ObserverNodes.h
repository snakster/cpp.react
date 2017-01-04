
//          Copyright Sebastian Jeckel 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_GRAPH_OBSERVERNODES_H_INCLUDED
#define REACT_DETAIL_GRAPH_OBSERVERNODES_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"
#include "react/API.h"

#include <memory>
#include <utility>

#include "GraphBase.h"

#include "react/detail/ReactiveInput.h"

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
    ObserverNode(const Group& group) :
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
        apply([this] (const auto& ... deps) { REACT_EXPAND_PACK(this->DetachFromMe(GetInternals(deps).GetNodeId())); }, depHolder_);
        this->UnregisterMe();
    }

    virtual const char* GetNodeType() const override
        { return "SignalObserver"; }

    virtual int GetDependencyCount() const override
        { return sizeof...(TDeps); }

    virtual UpdateResult Update(TurnId turnId, size_t successorCount) override
    {
        apply([this] (const auto& ... deps) { this->func_(GetInternals(deps).Value() ...); }, depHolder_);
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

    virtual const char* GetNodeType() const override
        { return "EventObserver"; }

    virtual int GetDependencyCount() const override
        { return 1; }

    virtual UpdateResult Update(TurnId turnId, size_t successorCount) override
    {
        func_(EventRange<E>( GetInternals(subject_).Events() ));
        GetInternals(subject_).DecrementPendingSuccessorCount();
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
        apply([this] (const auto& ... syncs) { REACT_EXPAND_PACK(this->DetachFromMe(GetInternals(syncs).GetNodeId())); }, syncHolder_);
        this->DetachFromMe(GetInternals(subject_).GetNodeId());
        this->UnregisterMe();
    }

    virtual const char* GetNodeType() const override
        { return "SyncedEventObserver"; }

    virtual int GetDependencyCount() const override
        { return 1 + sizeof...(TSyncs); }

    virtual UpdateResult Update(TurnId turnId, size_t successorCount) override
    {
        // Updates might be triggered even if only sync nodes changed. Ignore those.
        if (GetInternals(this->subject_).Events().empty())
            return UpdateResult::unchanged;

        apply([this] (const auto& ... syncs) { func_(EventRange<E>( GetInternals(this->subject_).Events() ), GetInternals(syncs).Value() ...); }, syncHolder_);

        GetInternals(subject_).DecrementPendingSuccessorCount();

        return UpdateResult::unchanged;
    }

private:
    F func_;

    Event<E> subject_;

    std::tuple<Signal<TSyncs> ...> syncHolder_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_GRAPH_OBSERVERNODES_H_INCLUDED