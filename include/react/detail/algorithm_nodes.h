
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_ALGORITHM_NODES_H_INCLUDED
#define REACT_DETAIL_ALGORITHM_NODES_H_INCLUDED

#pragma once

#include "react/detail/defs.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "state_nodes.h"
#include "event_nodes.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IterateNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename F, typename E>
class IterateNode : public StateNode<S>
{
public:
    template <typename T, typename FIn>
    IterateNode(const Group& group, T&& init, FIn&& func, const Event<E>& evnt) :
        IterateNode::StateNode( group, std::forward<T>(init) ),
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
        S newValue = func_(GetInternals(evnt_).Events(), this->Value());

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
class IterateByRefNode : public StateNode<S>
{
public:
    template <typename T, typename FIn>
    IterateByRefNode(const Group& group, T&& init, FIn&& func, const Event<E>& evnt) :
        IterateByRefNode::StateNode( group, std::forward<T>(init) ),
        func_( std::forward<FIn>(func) ),
        evnt_( evnt )
    {
        this->RegisterMe();
        this->AttachToMe(GetInternals(evnt_).GetNodeId());
    }

    ~IterateByRefNode()
    {
        this->DetachFromMe(GetInternals(evnt_).GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        func_(GetInternals(evnt_).Events(), this->Value());

        // Always assume a change
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
class SyncedIterateNode : public StateNode<S>
{
public:
    template <typename T, typename FIn>
    SyncedIterateNode(const Group& group, T&& init, FIn&& func, const Event<E>& evnt, const State<TSyncs>& ... syncs) :
        SyncedIterateNode::StateNode( group, std::forward<T>(init) ),
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
        apply([this] (const auto& ... syncs)
            { REACT_EXPAND_PACK(this->DetachFromMe(GetInternals(syncs).GetNodeId())); }, syncHolder_);
        this->DetachFromMe(GetInternals(evnt_).GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        // Updates might be triggered even if only sync nodes changed. Ignore those.
        if (GetInternals(evnt_).Events().empty())
            return UpdateResult::unchanged;

        S newValue = apply([this] (const auto& ... syncs)
            {
                return func_(GetInternals(evnt_).Events(), this->Value(), GetInternals(syncs).Value() ...);
            }, syncHolder_);

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

    std::tuple<State<TSyncs> ...> syncHolder_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncedIterateByRefNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename F, typename E, typename ... TSyncs>
class SyncedIterateByRefNode : public StateNode<S>
{
public:
    template <typename T, typename FIn>
    SyncedIterateByRefNode(const Group& group, T&& init, FIn&& func, const Event<E>& evnt, const State<TSyncs>& ... syncs) :
        SyncedIterateByRefNode::StateNode( group, std::forward<T>(init) ),
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
        apply([this] (const auto& ... syncs) { REACT_EXPAND_PACK(this->DetachFromMe(GetInternals(syncs).GetNodeId())); }, syncHolder_);
        this->DetachFromMe(GetInternals(evnt_).GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        // Updates might be triggered even if only sync nodes changed. Ignore those.
        if (GetInternals(evnt_).Events().empty())
            return UpdateResult::unchanged;

        apply(
            [this] (const auto& ... args)
            {
                func_(GetInternals(evnt_).Events(), this->Value(), GetInternals(args).Value() ...);
            },
            syncHolder_);

        return UpdateResult::changed;
    }

private:
    F           func_;
    Event<E>    evnt_;

    std::tuple<State<TSyncs> ...> syncHolder_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// HoldNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class HoldNode : public StateNode<S>
{
public:
    template <typename T>
    HoldNode(const Group& group, T&& init, const Event<S>& evnt) :
        HoldNode::StateNode( group, std::forward<T>(init) ),
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
class SnapshotNode : public StateNode<S>
{
public:
    SnapshotNode(const Group& group, const State<S>& target, const Event<E>& trigger) :
        SnapshotNode::StateNode( group, GetInternals(target).Value() ),
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
    State<S>    target_;
    Event<E>    trigger_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MonitorNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class MonitorNode : public EventNode<S>
{
public:
    MonitorNode(const Group& group, const State<S>& input) :
        MonitorNode::EventNode( group ),
        input_( input )
    {
        this->RegisterMe();
        this->AttachToMe(GetInternals(input_).GetNodeId());
    }

    ~MonitorNode()
    {
        this->DetachFromMe(GetInternals(input_).GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        this->Events().push_back(GetInternals(input_).Value());
        return UpdateResult::changed;
    }

private:
    State<S>    input_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// PulseNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename E>
class PulseNode : public EventNode<S>
{
public:
    PulseNode(const Group& group, const State<S>& input, const Event<E>& trigger) :
        PulseNode::EventNode( group ),
        input_( input ),
        trigger_( trigger )
    {
        this->RegisterMe();
        this->AttachToMe(GetInternals(input).GetNodeId());
        this->AttachToMe(GetInternals(trigger).GetNodeId());
    }

    ~PulseNode()
    {
        this->DetachFromMe(GetInternals(trigger_).GetNodeId());
        this->DetachFromMe(GetInternals(input_).GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        for (size_t i = 0; i < GetInternals(trigger_).Events().size(); ++i)
            this->Events().push_back(GetInternals(input_).Value());

        if (! this->Events().empty())
            return UpdateResult::changed;
        else
            return UpdateResult::unchanged;
    }

private:
    State<S>    input_;
    Event<E>    trigger_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// FlattenStateNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, template <typename> class TState>
class FlattenStateNode : public StateNode<S>
{
public:
    FlattenStateNode(const Group& group, const State<TState<S>>& outer) :
        FlattenStateNode::StateNode( group, GetInternals(GetInternals(outer).Value()).Value() ),
        outer_( outer ),
        inner_( GetInternals(outer).Value() )
    {
        this->RegisterMe();
        this->AttachToMe(GetInternals(outer_).GetNodeId());
        this->AttachToMe(GetInternals(inner_).GetNodeId());
    }

    ~FlattenStateNode()
    {
        this->DetachFromMe(GetInternals(inner_).GetNodeId());
        this->DetachFromMe(GetInternals(outer_).GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        const State<S>& newInner = GetInternals(outer_).Value();

        // Check if there's a new inner node.
        if (! (newInner == inner_))
        {
            this->DetachFromMe(GetInternals(inner_).GetNodeId());
            this->AttachToMe(GetInternals(newInner).GetNodeId());
            inner_ = newInner;
            return UpdateResult::shifted;
        }

        const S& newValue = GetInternals(inner_).Value();

        if (HasChanged(newValue, this->Value()))
        {
            this->Value() = newValue;
            return UpdateResult::changed;
        }

        return UpdateResult::unchanged;
    }

private:
    State<TState<S>>    outer_;
    State<S>            inner_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// FlattenStateListNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <template <typename ...> class TList, template <typename> class TState, typename V, typename ... TParams>
class FlattenStateListNode : public StateNode<TList<V>>
{
public:
    using InputListType = TList<TState<V>, TParams ...>;
    using FlatListType = TList<V>;

    FlattenStateListNode(const Group& group, const State<InputListType>& outer) :
        FlattenStateListNode::StateNode( group, MakeFlatList(GetInternals(outer).Value()) ),
        outer_( outer ),
        inner_( GetInternals(outer).Value() )
    {
        this->RegisterMe();
        this->AttachToMe(GetInternals(outer_).GetNodeId());

        for (const State<V>& state : inner_)
            this->AttachToMe(GetInternals(state).GetNodeId());
    }

    ~FlattenStateListNode()
    {
        for (const State<V>& state : inner_)
            this->DetachFromMe(GetInternals(state).GetNodeId());

        this->DetachFromMe(GetInternals(outer_).GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        const InputListType& newInner = GetInternals(outer_).Value();

        // Check if there's a new inner node.
        if (! (std::equal(begin(newInner), end(newInner), begin(inner_), end(inner_))))
        {
            for (const State<V>& state : inner_)
                this->DetachFromMe(GetInternals(state).GetNodeId());
            for (const State<V>& state : newInner)
                this->AttachToMe(GetInternals(state).GetNodeId());

            inner_ = newInner;
            return UpdateResult::shifted;
        }

        FlatListType newValue = MakeFlatList(inner_);
        const FlatListType& curValue = this->Value();

        if (! (std::equal(begin(newValue), end(newValue), begin(curValue), end(curValue), [] (const auto& a, const auto& b)
            { return !HasChanged(a, b); })))
        {
            this->Value() = std::move(newValue);
            return UpdateResult::changed;
        }

        return UpdateResult::unchanged;
    }

private:
    static FlatListType MakeFlatList(const InputListType& list)
    {
        FlatListType res;
        for (const State<V>& state : list)
            ListInsert(res, GetInternals(state).Value());
        return res;
    }

    State<InputListType>    outer_;
    InputListType           inner_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// FlattenStateListNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <template <typename ...> class TMap, template <typename> class TState, typename K, typename V, typename ... TParams>
class FlattenStateMapNode : public StateNode<TMap<K, V>>
{
public:
    using InputMapType = TMap<K, TState<V>, TParams ...>;
    using FlatMapType = TMap<K, V>;

    FlattenStateMapNode(const Group& group, const State<InputMapType>& outer) :
        FlattenStateMapNode::StateNode( group, MakeFlatMap(GetInternals(outer).Value()) ),
        outer_( outer ),
        inner_( GetInternals(outer).Value() )
    {
        this->RegisterMe();
        this->AttachToMe(GetInternals(outer_).GetNodeId());

        for (const auto& entry : inner_)
            this->AttachToMe(GetInternals(entry.second).GetNodeId());
    }

    ~FlattenStateMapNode()
    {
        for (const auto& entry : inner_)
            this->DetachFromMe(GetInternals(entry.second).GetNodeId());

        this->DetachFromMe(GetInternals(outer_).GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        const InputMapType& newInner = GetInternals(outer_).Value();

        // Check if there's a new inner node.
        if (! (std::equal(begin(newInner), end(newInner), begin(inner_), end(inner_))))
        {
            for (const auto& entry : inner_)
                this->DetachFromMe(GetInternals(entry.second).GetNodeId());
            for (const auto& entry : newInner)
                this->AttachToMe(GetInternals(entry.second).GetNodeId());

            inner_ = newInner;
            return UpdateResult::shifted;
        }

        FlatMapType newValue = MakeFlatMap(inner_);
        const FlatMapType& curValue = this->Value();

        if (! (std::equal(begin(newValue), end(newValue), begin(curValue), end(curValue), [] (const auto& a, const auto& b)
            { return !HasChanged(a.first, b.first) && !HasChanged(a.second, b.second); })))
        {
            this->Value() = std::move(newValue);
            return UpdateResult::changed;
        }

        return UpdateResult::unchanged;
    }

private:
    static FlatMapType MakeFlatMap(const InputMapType& map)
    {
        FlatMapType res;
        for (const auto& entry : map)
            MapInsert(res, typename FlatMapType::value_type{ entry.first, GetInternals(entry.second).Value() });
        return res;
    }

    State<InputMapType>    outer_;
    InputMapType           inner_;
};

struct FlattenedInitTag { };

///////////////////////////////////////////////////////////////////////////////////////////////////
/// FlattenObjectNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T, typename TFlat>
class FlattenObjectNode : public StateNode<TFlat>
{
public:
    FlattenObjectNode(const Group& group, const State<T>& obj) :
        StateNode::StateNode( in_place, group, GetInternals(obj).Value(), FlattenedInitTag{ } ),
        obj_( obj )
    {
        this->RegisterMe();
        this->AttachToMe(GetInternals(obj).GetNodeId());

        for (NodeId nodeId : Value().memberIds_)
            this->AttachToMe(nodeId);

        this->Value().initMode_ = false;
    }

    ~FlattenObjectNode()
    {
        for (NodeId nodeId :  Value().memberIds_)
            this->DetachFromMe(nodeId);

        this->DetachFromMe(GetInternals(obj_).GetNodeId());
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        const T& newValue = GetInternals(obj_).Value();

        if (HasChanged(newValue, static_cast<const T&>(this->Value())))
        {
            for (NodeId nodeId : this->Value().memberIds_)
                this->DetachFromMe(nodeId);

            // Steal array from old value for new value so we don't have to re-allocate.
            // The old value will freed after the assignment.
            this->Value().memberIds_.clear();
            this->Value() = TFlat { newValue, FlattenedInitTag{ }, std::move(this->Value().memberIds_) };

            for (NodeId nodeId : this->Value().memberIds_)
                this->AttachToMe(nodeId);

            return UpdateResult::shifted;
        }

        return UpdateResult::changed;
    }
private:
    State<T> obj_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_ALGORITHM_NODES_H_INCLUDED