
//          Copyright Sebastian Jeckel 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_GRAPH_OBSERVERNODES_H_INCLUDED
#define REACT_DETAIL_GRAPH_OBSERVERNODES_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

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
    ObserverNode(const std::shared_ptr<ReactiveGraph>& graphPtr) :
        ObserverNode::NodeBase( graphPtr )
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
    SignalObserverNode(const std::shared_ptr<ReactiveGraph>& graphPtr, FIn&& func, const std::shared_ptr<SignalNode<TDeps>>& ... deps) :
        SignalObserverNode::ObserverNode( graphPtr ),
        func_( std::forward<FIn>(func) ),
        depHolder_( deps ... )
    {
        this->RegisterMe(NodeCategory::output);
        REACT_EXPAND_PACK(this->AttachToMe(deps->GetNodeId()));
    }

    ~SignalObserverNode()
    {
        apply([this] (const auto& ... deps) { REACT_EXPAND_PACK(this->DetachFromMe(deps->GetNodeId())); }, depHolder_);
        this->UnregisterMe();
    }

    virtual const char* GetNodeType() const override
        { return "SignalObserver"; }

    virtual int GetDependencyCount() const override
        { return sizeof...(TDeps); }

    virtual UpdateResult Update(TurnId turnId, int successorCount) override
    {
        apply([this] (const auto& ... deps) { this->func_(deps->Value() ...); }, depHolder_);
        return UpdateResult::unchanged;
    }

private:
    F func_;

    std::tuple<std::shared_ptr<SignalNode<TDeps>> ...> depHolder_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventObserverNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename F, typename E>
class EventObserverNode : public ObserverNode
{
public:
    template <typename FIn>
    EventObserverNode(const std::shared_ptr<ReactiveGraph>& graphPtr, FIn&& func, const std::shared_ptr<EventStreamNode<E>>& subject) :
        EventObserverNode::ObserverNode( graphPtr ),
        func_( std::forward<FIn>(func) ),
        subject_( subject )
    {
        this->RegisterMe(NodeCategory::output);
        this->AttachToMe(subject->GetNodeId());
    }

    ~EventObserverNode()
    {
        this->DetachFromMe(subject_->GetNodeId());
        this->UnregisterMe();
    }

    virtual const char* GetNodeType() const override
        { return "EventObserver"; }

    virtual int GetDependencyCount() const override
        { return 1; }

    virtual UpdateResult Update(TurnId turnId, int successorCount) override
    {
        func_(EventRange<E>( subject_->Events() ));
        subject_->DecrementPendingSuccessorCount();
        return UpdateResult::unchanged;
    }

private:
    F func_;

    std::shared_ptr<EventStreamNode<E>> subject_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncedEventObserverNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename F, typename E, typename ... TSyncs>
class SyncedEventObserverNode : public ObserverNode
{
public:
    template <typename FIn>
    SyncedEventObserverNode(const std::shared_ptr<ReactiveGraph>& graphPtr, FIn&& func, const std::shared_ptr<EventStreamNode<E>>& subject, const std::shared_ptr<SignalNode<TSyncs>>& ... syncs) :
        SyncedEventObserverNode::ObserverNode( graphPtr ),
        func_( std::forward<FIn>(func) ),
        subject_( subject ),
        syncHolder_( syncs ... )
    {
        this->RegisterMe(NodeCategory::output);
        this->AttachToMe(subject->GetNodeId());
        REACT_EXPAND_PACK(this->AttachToMe(syncs->GetNodeId()));
    }

    ~SyncedEventObserverNode()
    {
        apply([this] (const auto& ... syncs) { REACT_EXPAND_PACK(this->DetachFromMe(syncs->GetNodeId())); }, syncHolder_);
        this->DetachFromMe(subject_->GetNodeId());
        this->UnregisterMe();
    }

    virtual const char* GetNodeType() const override
        { return "SyncedEventObserver"; }

    virtual int GetDependencyCount() const override
        { return 1 + sizeof...(TSyncs); }

    virtual UpdateResult Update(TurnId turnId, int successorCount) override
    {
        // Updates might be triggered even if only sync nodes changed. Ignore those.
        if (subject_->Events().empty())
            return UpdateResult::unchanged;

        apply([this] (const auto& ... syncs) { func_(EventRange<E>( this->subject_->Events() ), syncs->Value() ...); }, syncHolder_);

        subject_->DecrementPendingSuccessorCount();

        return UpdateResult::unchanged;
    }

private:
    F func_;

    std::shared_ptr<EventStreamNode<E>> subject_;

    std::tuple<std::shared_ptr<SignalNode<TSyncs>>...> syncHolder_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_GRAPH_OBSERVERNODES_H_INCLUDED