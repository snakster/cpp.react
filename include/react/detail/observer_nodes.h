
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_OBSERVER_NODES_H_INCLUDED
#define REACT_DETAIL_OBSERVER_NODES_H_INCLUDED

#pragma once

#include "react/detail/defs.h"
#include "react/api.h"
#include "react/common/utility.h"

#include <memory>
#include <utility>
#include <tuple>

#include "node_base.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward declarations
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class StateNode;

template <typename E>
class EventStreamNode;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// StateObserverNode
///////////////////////////////////////////////////////////////////////////////////////////////////
class ObserverNode : public NodeBase
{
public:
    explicit ObserverNode(const Group& group) :
        ObserverNode::NodeBase( group )
    { }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// StateObserverNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename F, typename ... TDeps>
class StateObserverNode : public ObserverNode
{
public:
    template <typename FIn>
    StateObserverNode(const Group& group, FIn&& func, const State<TDeps>& ... deps) :
        StateObserverNode::ObserverNode( group ),
        func_( std::forward<FIn>(func) ),
        depHolder_( deps ... )
    {
        this->RegisterMe(NodeCategory::output);
        REACT_EXPAND_PACK(this->AttachToMe(GetInternals(deps).GetNodeId()));

        apply([this] (const auto& ... deps)
            { this->func_(GetInternals(deps).Value() ...); }, depHolder_);
    }

    ~StateObserverNode()
    {
        apply([this] (const auto& ... deps)
            { REACT_EXPAND_PACK(this->DetachFromMe(GetInternals(deps).GetNodeId())); }, depHolder_);
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        apply([this] (const auto& ... deps)
            { this->func_(GetInternals(deps).Value() ...); }, depHolder_);
        return UpdateResult::unchanged;
    }

private:
    F func_;

    std::tuple<State<TDeps> ...> depHolder_;
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
    SyncedEventObserverNode(const Group& group, FIn&& func, const Event<E>& subject, const State<TSyncs>& ... syncs) :
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
        apply([this] (const auto& ... syncs)
            { REACT_EXPAND_PACK(this->DetachFromMe(GetInternals(syncs).GetNodeId())); }, syncHolder_);
        this->DetachFromMe(GetInternals(subject_).GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        // Updates might be triggered even if only sync nodes changed. Ignore those.
        if (GetInternals(this->subject_).Events().empty())
            return UpdateResult::unchanged;

        apply([this] (const auto& ... syncs)
            { func_(GetInternals(this->subject_).Events(), GetInternals(syncs).Value() ...); }, syncHolder_);

        return UpdateResult::unchanged;
    }

private:
    F func_;

    Event<E> subject_;

    std::tuple<State<TSyncs> ...> syncHolder_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ObserverInternals
///////////////////////////////////////////////////////////////////////////////////////////////////
class ObserverInternals
{
public:
    ObserverInternals(const ObserverInternals&) = default;
    ObserverInternals& operator=(const ObserverInternals&) = default;

    ObserverInternals(ObserverInternals&&) = default;
    ObserverInternals& operator=(ObserverInternals&&) = default;

    explicit ObserverInternals(std::shared_ptr<ObserverNode>&& nodePtr) :
        nodePtr_( std::move(nodePtr) )
    { }

    auto GetNodePtr() -> std::shared_ptr<ObserverNode>&
        { return nodePtr_; }

    auto GetNodePtr() const -> const std::shared_ptr<ObserverNode>&
        { return nodePtr_; }

    NodeId GetNodeId() const
        { return nodePtr_->GetNodeId(); }

protected:
    ObserverInternals() = default;

private:
    std::shared_ptr<ObserverNode> nodePtr_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_OBSERVER_NODES_H_INCLUDED