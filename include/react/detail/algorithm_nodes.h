
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_ALGORITHM_NODES_H_INCLUDED
#define REACT_DETAIL_ALGORITHM_NODES_H_INCLUDED

#pragma once

#include "react/detail/defs.h"

#include <memory>
#include <utility>

#include "event_nodes.h"
#include "signal_nodes.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IterateNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename F, typename E>
class IterateNode : public SignalNode<S>
{
public:
    template <typename T, typename FIn>
    IterateNode(const Group& group, T&& init, FIn&& func, const Event<E>& evnt) :
        IterateNode::SignalNode( group, std::forward<T>(init) ),
        func_( std::forward<FIn>(func) ),
        evnt_( evnt )
    {
        this->RegisterMe();
        this->AttachToMe(GetInternals(evnt).GetNodeId());
    }

    ~IterateNode()
    {
        this->DetachFromMe(GetInternals(evnt_).GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        S newValue = func_(EventRange<E>( GetInternals(evnt_).Events() ), this->Value());

        if (! (newValue == this->Value()))
        {
            this->Value() = std::move(newValue);
            return UpdateResult::changed;
        }
        else
        {
            return UpdateResult::unchanged;
        }
    }

private:
    F           func_;
    Event<E>    evnt_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IterateByRefNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename F, typename E>
class IterateByRefNode : public SignalNode<S>
{
public:
    template <typename T, typename FIn>
    IterateByRefNode(const Group& group, T&& init, FIn&& func, const Event<E>& evnt) :
        IterateByRefNode::SignalNode( group, std::forward<T>(init) ),
        func_( std::forward<FIn>(func) ),
        evnt_( evnt )
    {
        this->RegisterMe();
        this->AttachToMe(GetInternals(evnt_).GetNodeId());
    }

    ~IterateByRefNode()
    {
        this->DetachFromMe(GetInternals(evnt).GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        func_(EventRange<E>( GetInternals(evnt_).Events() ), this->Value());

        // Always assume change
        return UpdateResult::changed;
    }

protected:
    F           func_;
    Event<E>    evnt_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncedIterateNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename F, typename E, typename ... TSyncs>
class SyncedIterateNode : public SignalNode<S>
{
public:
    template <typename T, typename FIn>
    SyncedIterateNode(const Group& group, T&& init, FIn&& func, const Event<E>& evnt, const Signal<TSyncs>& ... syncs) :
        SyncedIterateNode::SignalNode( group, std::forward<T>(init) ),
        func_( std::forward<FIn>(func) ),
        evnt_( evnt ),
        syncHolder_( syncs ... )
    {
        this->RegisterMe();
        this->AttachToMe(GetInternals(evnt).GetNodeId());
        REACT_EXPAND_PACK(this->AttachToMe(GetInternals(syncs).GetNodeId()));
    }

    ~SyncedIterateNode()
    {
        std::apply([this] (const auto& ... syncs) { REACT_EXPAND_PACK(this->DetachFromMe(GetInternals(syncs).GetNodeId())); }, syncHolder_);
        this->DetachFromMe(GetInternals(evnt_).GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        // Updates might be triggered even if only sync nodes changed. Ignore those.
        if (GetInternals(evnt_).Events().empty())
            return UpdateResult::unchanged;

        S newValue = std::apply(
            [this] (const auto& ... syncs)
            {
                return func_(EventRange<E>( GetInternals(evnt_).Events() ), this->Value(), GetInternals(syncs).Value() ...);
            },
            syncHolder_);

        if (! (newValue == this->Value()))
        {
            this->Value() = std::move(newValue);
            return UpdateResult::changed;
        }
        else
        {
            return UpdateResult::unchanged;
        }   
    }

private:
    F           func_;
    Event<E>    evnt_;

    std::tuple<Signal<TSyncs> ...> syncHolder_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncedIterateByRefNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename F, typename E, typename ... TSyncs>
class SyncedIterateByRefNode : public SignalNode<S>
{
public:
    template <typename T, typename FIn>
    SyncedIterateByRefNode(const Group& group, T&& init, FIn&& func, const Event<E>& evnt, const Signal<TSyncs>& ... syncs) :
        SyncedIterateByRefNode::SignalNode( group, std::forward<T>(init) ),
        func_( std::forward<FIn>(func) ),
        evnt_( evnt ),
        syncHolder_( syncs ... )
    {
        this->RegisterMe();
        this->AttachToMe(GetInternals(evnt).GetNodeId());
        REACT_EXPAND_PACK(this->AttachToMe(GetInternals(syncs).GetNodeId()));
    }

    ~SyncedIterateByRefNode()
    {
        std::apply([this] (const auto& ... syncs) { REACT_EXPAND_PACK(this->DetachFromMe(GetInternals(syncs).GetNodeId())); }, syncHolder_);
        this->DetachFromMe(GetInternals(evnt_).GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        // Updates might be triggered even if only sync nodes changed. Ignore those.
        if (GetInternals(evnt_).Events().empty())
            return UpdateResult::unchanged;

        std::apply(
            [this] (const auto& ... args)
            {
                func_(EventRange<E>( GetInternals(evnt_).Events() ), this->Value(), GetInternals(args).Value() ...);
            },
            syncHolder_);

        return UpdateResult::changed;
    }

private:
    F           func_;
    Event<E>    events_;

    std::tuple<Signal<TSyncs> ...> syncHolder_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// HoldNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class HoldNode : public SignalNode<S>
{
public:
    template <typename T>
    HoldNode(const Group& group, T&& init, const Event<S>& evnt) :
        HoldNode::SignalNode( group, std::forward<T>(init) ),
        evnt_( evnt )
    {
        this->RegisterMe();
        this->AttachToMe(GetInternals(evnt).GetNodeId());
    }

    ~HoldNode()
    {
        this->DetachFromMe(GetInternals(evnt_).GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        bool changed = false;

        if (! GetInternals(evnt_).Events().empty())
        {
            const S& newValue = GetInternals(evnt_).Events().back();

            if (! (newValue == this->Value()))
            {
                changed = true;
                this->Value() = newValue;
            }
        }

        if (changed)
            return UpdateResult::changed;
        else
            return UpdateResult::unchanged;
    }

private:
    Event<S>  evnt_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SnapshotNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename E>
class SnapshotNode : public SignalNode<S>
{
public:
    SnapshotNode(const Group& group, const Signal<S>& target, const Event<E>& trigger) :
        SnapshotNode::SignalNode( group, GetInternals(target).Value() ),
        target_( target ),
        trigger_( trigger )
    {
        this->RegisterMe();
        this->AttachToMe(GetInternals(target).GetNodeId());
        this->AttachToMe(GetInternals(trigger).GetNodeId());
    }

    ~SnapshotNode()
    {
        this->DetachFromMe(GetInternals(trigger_).GetNodeId());
        this->DetachFromMe(GetInternals(target_).GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        bool changed = false;
        
        if (! GetInternals(trigger_).Events().empty())
        {
            const S& newValue = GetInternals(target_).Value();

            if (! (newValue == this->Value()))
            {
                changed = true;
                this->Value() = newValue;
            }
        }

        if (changed)
            return UpdateResult::changed;
        else
            return UpdateResult::unchanged;
    }

private:
    Signal<S>   target_;
    Event<E>    trigger_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MonitorNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class MonitorNode : public EventStreamNode<S>
{
public:
    MonitorNode(const Group& group, const Signal<S>& target) :
        MonitorNode::EventStreamNode( group ),
        target_( target )
    {
        this->RegisterMe();
        this->AttachToMe(GetInternals(target).GetNodeId());
    }

    ~MonitorNode()
    {
        this->DetachFromMe(GetInternals(target_).GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        this->Events().push_back(GetInternals(target_).Value());

        return UpdateResult::changed;
    }

private:
    Signal<S>    target_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// PulseNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename E>
class PulseNode : public EventStreamNode<S>
{
public:
    PulseNode(const Group& group, const Signal<S>& target, const Event<E>& trigger) :
        PulseNode::EventStreamNode( group ),
        target_( target ),
        trigger_( trigger )
    {
        this->RegisterMe();
        this->AttachToMe(GetInternals(target).GetNodeId());
        this->AttachToMe(GetInternals(trigger).GetNodeId());
    }

    ~PulseNode()
    {
        this->DetachFromMe(GetInternals(trigger_).GetNodeId());
        this->DetachFromMe(GetInternals(target_).GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        for (size_t i = 0; i < GetInternals(trigger_).Events().size(); i++)
            this->Events().push_back(GetInternals(target_).Value());

        if (! this->Events().empty())
        {
            return UpdateResult::changed;
        }
        else
        {
            return UpdateResult::unchanged;
        }
    }

private:
    Signal<S>   target_;
    Event<E>    trigger_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_ALGORITHM_NODES_H_INCLUDED