
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
template <typename T>
class SignalNode;

template <typename T>
class EventStreamNode;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ObserverNode
///////////////////////////////////////////////////////////////////////////////////////////////////
class ObserverNode : public NodeBase, public IObserver
{
public:
    ObserverNode(IReactiveGraph* group) : NodeBase( group )
        { }

    ObserverNode(ObserverNode&&) = default;
    ObserverNode& operator=(ObserverNode&&) = default;

    ObserverNode(const ObserverNode&) = delete;
    ObserverNode& operator=(const ObserverNode&) = delete;

    virtual bool IsOutputNode() const
        { return true; }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SignalObserverNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T, typename F>
class SignalObserverNode : public ObserverNode
{
public:
    template <typename FIn>
    SignalObserverNode(IReactiveGraph* group, const std::shared_ptr<SignalNode<T>>& subject, FIn&& func) :
        SignalObserverNode::ObserverNode( group ),
        subject_( subject ),
        func_( std::forward<FIn>(func) )
    {
        this->RegisterMe();
        this->AttachToMe(subject->GetNodeId());
    }

    ~SignalObserverNode()
    {
        this->DetachFromMe(subject->GetNodeId());
        this->UnregisterMe();
    }

    virtual const char* GetNodeType() const override
        { return "SignalObserver"; }

    virtual int DependencyCount() const override
        { return 1; }

    virtual UpdateResult Update(TurnId turnId) override
    {
        func_(subject_->Value());
        return UpdateResult::unchanged;
    }

private:
    std::shared_ptr<SignalNode<T>> subject_;

    F func_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventObserverNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T, typename F>
class EventObserverNode : public ObserverNode
{
public:
    template <typename FIn>
    EventObserverNode(IReactiveGraph* group, const std::shared_ptr<EventStreamNode<T>>& subject, FIn&& func) :
        EventObserverNode::ObserverNode( group ),
        subject_( subject ),
        func_( std::forward<FIn>(func) )
    {
        this->RegisterMe();
        this->AttachToMe(subject->GetNodeId());
    }

    ~EventObserverNode()
    {
        this->DetachFromMe(subject->GetNodeId());
        this->UnregisterMe();
    }

    virtual const char* GetNodeType() const override
        { return "EventObserverNode"; }

    virtual int DependencyCount() const override
        { return 1; }

    virtual UpdateResult Update(TurnId turnId) override
    {
        func_(EventRange<T>( subject_->Events() ));
        return UpdateResult::unchanged;
    }

private:
    std::shared_ptr<EventStreamNode<T>> subject_;

    F func_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncedObserverNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T, typename F, typename ... TSyncs>
class SyncedObserverNode : public ObserverNode
{
public:
    template <typename FIn>
    SyncedObserverNode(IReactiveGraph* group, const std::shared_ptr<EventStreamNode<T>>& subject, FIn&& func, const std::shared_ptr<SignalNode<TSyncs>>& ... syncs) :
        SyncedObserverNode::ObserverNode( group ),
        subject_( subject ),
        func_( std::forward<FIn>(func) ),
        syncHolder_( syncs ... )
    {
        this->RegisterMe();
        this->AttachToMe(subject->GetNodeId());
        REACT_EXPAND_PACK(this->AttachToMe(syncs->GetNodeId()));
    }

    ~SyncedObserverNode()
    {
        apply([this] (const auto& ... syncs) { REACT_EXPAND_PACK(this->DetachFromMe(syncs->GetNodeId())); }, syncHolder_);
        this->DetachFromMe(subject_->GetNodeId());
        this->UnregisterMe();
    }

    virtual const char* GetNodeType() const override
        { return "SyncedObserverNode"; }

    virtual int DependencyCount() const override
        { return 1 + sizeof...(TSyncs); }

    virtual UpdateResult Update(TurnId turnId) override
    {   
        // Update of this node could be triggered from deps,
        // so make sure source doesnt contain events from last turn
        p->SetCurrentTurn(turnId);
            
        shouldDetach = apply(
            [this] (const auto& ... syncs)
            {
                func_(EventRange<E>( this->subject_->Events() ), syncs->Value() ...);
            },
            syncHolder_);

        return UpdateResult::unchanged;
    }

private:
    std::shared_ptr<EventStreamNode<T>> subject_;
    
    F func_;

    std::tuple<std::shared_ptr<SignalNode<TSyncs>>...> syncHolder_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_GRAPH_OBSERVERNODES_H_INCLUDED