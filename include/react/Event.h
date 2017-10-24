
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_EVENT_H_INCLUDED
#define REACT_EVENT_H_INCLUDED

#pragma once

#include "react/detail/defs.h"
#include "react/api.h"
#include "react/group.h"
#include "react/common/ptrcache.h"

#include <memory>
#include <type_traits>
#include <utility>

#include "react/detail/event_nodes.h"



/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Event
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E>
class Event : protected REACT_IMPL::EventInternals<E>
{
public:
    Event(const Event&) = default;
    Event& operator=(const Event&) = default;

    Event(Event&&) = default;
    Event& operator=(Event&&) = default;

    // Construct with explicit group
    template <typename F, typename T>
    Event(const Group& group, F&& func, const Event<T>& dep) :
        Event::Event( CreateProcessingNode(group, std::forward<F>(func), dep) )
    { }

    // Construct with implicit group
    template <typename F, typename T>
    Event(F&& func, const Event<T>& dep) :
        Event::Event( CreateProcessingNode(dep.GetGroup(), std::forward<F>(func), dep) )
    { }

    // Construct with explicit group
    template <typename F, typename T, typename ... Us>
    Event(const Group& group, F&& func, const Event<T>& dep, const State<Us>& ... states) :
        Event::Event( CreateSyncedProcessingNode(group, std::forward<F>(func), dep, states ...) )
    { }

    // Construct with implicit group
    template <typename F, typename T, typename ... Us>
    Event(F&& func, const Event<T>& dep, const State<Us>& ... states) :
        Event::Event( CreateSyncedProcessingNode(dep.GetGroup(), std::forward<F>(func), dep, states ...) )
    { }

    auto Tokenize() const -> decltype(auto)
        { return REACT::Tokenize(*this); }

    auto GetGroup() const -> const Group&
        { return GetNodePtr()->GetGroup(); }

    auto GetGroup() -> Group&
        { return GetNodePtr()->GetGroup(); }

    friend bool operator==(const Event<E>& a, const Event<E>& b)
        { return a.GetNodePtr() == b.GetNodePtr(); }

    friend bool operator!=(const Event<E>& a, const Event<E>& b)
        { return !(a == b); }

    friend auto GetInternals(Event<E>& e) -> REACT_IMPL::EventInternals<E>&
        { return e; }

    friend auto GetInternals(const Event<E>& e) -> const REACT_IMPL::EventInternals<E>&
        { return e; }

protected:
    // Private node ctor
    explicit Event(std::shared_ptr<REACT_IMPL::EventNode<E>>&& nodePtr) :
        Event::EventInternals( std::move(nodePtr) )
    { }

    template <typename F, typename T>
    auto CreateProcessingNode(const Group& group, F&& func, const Event<T>& dep) -> decltype(auto)
    {
        using REACT_IMPL::EventProcessingNode;
        using REACT_IMPL::SameGroupOrLink;

        return std::make_shared<EventProcessingNode<E, T, typename std::decay<F>::type>>(
            group, std::forward<F>(func), SameGroupOrLink(group, dep));
    }

    template <typename F, typename T, typename ... Us>
    auto CreateSyncedProcessingNode(const Group& group, F&& func, const Event<T>& dep, const State<Us>& ... syncs) -> decltype(auto)
    {
        using REACT_IMPL::SyncedEventProcessingNode;
        using REACT_IMPL::SameGroupOrLink;

        return std::make_shared<SyncedEventProcessingNode<E, T, typename std::decay<F>::type, Us ...>>(
            group, std::forward<F>(func), SameGroupOrLink(group, dep), SameGroupOrLink(group, syncs) ...);
    }

    template <typename RET, typename NODE, typename ... ARGS>
    friend static RET impl::CreateWrappedNode(ARGS&& ... args);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventSource
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E>
class EventSource : public Event<E>
{
public:
    EventSource(const EventSource&) = default;
    EventSource& operator=(const EventSource&) = default;

    EventSource(EventSource&& other) = default;
    EventSource& operator=(EventSource&& other) = default;

    // Construct event source
    explicit EventSource(const Group& group) :
        EventSource::Event( CreateSourceNode(group) )
    { }
    
    void Emit(const E& value)
        { EmitValue(value); }

    void Emit(E&& value)
        { EmitValue(std::move(value)); }

    template <typename T = E, typename = std::enable_if_t<std::is_same_v<T,Token>>::type>
    void Emit()
        { EmitValue(Token::value); }

    EventSource& operator<<(const E& value)
        { EmitValue(e); return *this; }

    EventSource& operator<<(E&& value)
        { EmitValue(std::move(value)); return *this; }

protected:
    auto CreateSourceNode(const Group& group) -> decltype(auto)
    {
        using REACT_IMPL::EventSourceNode;
        return std::make_shared<EventSourceNode<E>>(group);
    }

private:
    template <typename T>
    void EmitValue(T&& value)
    {
        using REACT_IMPL::NodeId;
        using REACT_IMPL::ReactGraph;
        using REACT_IMPL::EventSourceNode;

        auto* castedPtr = static_cast<EventSourceNode<E>*>(this->GetNodePtr().get());

        NodeId nodeId = castedPtr->GetNodeId();
        auto& graphPtr = GetInternals(this->GetGroup()).GetGraphPtr();

        graphPtr->PushInput(nodeId, [castedPtr, &value] { castedPtr->EmitValue(std::forward<T>(value)); });
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventSlotBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E>
class EventSlot : public Event<E>
{
public:
    EventSlot(const EventSlot&) = default;
    EventSlot& operator=(const EventSlot&) = default;

    EventSlot(EventSlot&&) = default;
    EventSlot& operator=(EventSlot&&) = default;

    // Construct emtpy slot
    EventSlot(const Group& group) :
        EventSlot::Event( CreateSlotNode(group) )
    { }

    void Add(const Event<E>& input)
        { AddInput(input); }

    void Remove(const Event<E>& input)
        { RemoveInput(input); }

    void RemoveAll()
        { RemoveAllInputs(); }

protected:
    auto CreateSlotNode(const Group& group) -> decltype(auto)
    {
        using REACT_IMPL::EventSlotNode;
        return std::make_shared<EventSlotNode<E>>(group);
    }

private:
    void AddInput(const Event<E>& input)
    {
        using REACT_IMPL::NodeId;
        using SlotNodeType = REACT_IMPL::EventSlotNode<E>;

        SlotNodeType* castedPtr = static_cast<SlotNodeType*>(this->GetNodePtr().get());

        NodeId nodeId = castedPtr->GetInputNodeId();
        auto& graphPtr = GetInternals(this->GetGroup()).GetGraphPtr();

        graphPtr->PushInput(nodeId, [this, castedPtr, &input] { castedPtr->AddInput(SameGroupOrLink(GetGroup(), input)); });
    }

    void RemoveInput(const Event<E>& input)
    {
        using REACT_IMPL::NodeId;
        using SlotNodeType = REACT_IMPL::EventSlotNode<E>;

        SlotNodeType* castedPtr = static_cast<SlotNodeType*>(this->GetNodePtr().get());

        NodeId nodeId = castedPtr->GetInputNodeId();
        auto& graphPtr = GetInternals(this->GetGroup()).GetGraphPtr();

        graphPtr->PushInput(nodeId, [this, castedPtr, &input] { castedPtr->RemoveInput(SameGroupOrLink(GetGroup(), input)); });
    }

    void RemoveAllInputs()
    {
        using REACT_IMPL::NodeId;
        using SlotNodeType = REACT_IMPL::EventSlotNode<E>;

        SlotNodeType* castedPtr = static_cast<SlotNodeType*>(this->GetNodePtr().get());

        NodeId nodeId = castedPtr->GetInputNodeId();
        auto& graphPtr = GetInternals(this->GetGroup()).GetGraphPtr();

        graphPtr->PushInput(nodeId, [castedPtr] { castedPtr->RemoveAllInputs(); });
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventLink
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E>
class EventLink : public Event<E>
{
public:
    EventLink(const EventLink&) = default;
    EventLink& operator=(const EventLink&) = default;

    EventLink(EventLink&&) = default;
    EventLink& operator=(EventLink&&) = default;

    // Construct with group
    EventLink(const Group& group, const Event<E>& input) :
        EventLink::Event( GetOrCreateLinkNode(group, input) )
    { }

protected:
    static auto GetOrCreateLinkNode(const Group& group, const Event<E>& input) -> decltype(auto)
    {
        using REACT_IMPL::EventLinkNode;
        using REACT_IMPL::IReactNode;
        using REACT_IMPL::ReactGraph;
        
        IReactNode* k = GetInternals(input).GetNodePtr().get();

        ReactGraph::LinkCache& linkCache = GetInternals(group).GetGraphPtr()->GetLinkCache();

        std::shared_ptr<IReactNode> nodePtr = linkCache.LookupOrCreate(
            k,
            [&]
            {
                auto nodePtr = std::make_shared<EventLinkNode<E>>(group, input);
                nodePtr->SetWeakSelfPtr(std::weak_ptr<EventLinkNode<E>>{ nodePtr });
                return std::static_pointer_cast<IReactNode>(nodePtr);
            });

        return std::static_pointer_cast<EventLinkNode<E>>(nodePtr);
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Merge
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E, typename ... Us>
auto Merge(const Group& group, const Event<E>& dep1, const Event<Us>& ... deps) -> decltype(auto)
{
    using REACT_IMPL::EventMergeNode;
    using REACT_IMPL::SameGroupOrLink;

    return Event<E>::CreateWithNode<EventMergeNode<E, U1, Us ...>>(group, SameGroupOrLink(group, dep1), SameGroupOrLink(group, deps) ...);
}

template <typename T = void, typename U1, typename ... Us>
auto Merge(const Event<U1>& dep1, const Event<Us>& ... deps) -> decltype(auto)
    { return Merge(dep1.GetGroup(), dep1, deps ...); }

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Filter
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename F, typename E>
auto Filter(const Group& group, F&& pred, const Event<E>& dep) -> Event<E>
{
    auto filterFunc = [capturedPred = std::forward<F>(pred)] (const EventValueList<E>& evts, EventValueSink<E> out)
        { std::copy_if(evts.begin(), evts.end(), out, capturedPred); };

    return Event<E>(group, std::move(filterFunc), dep);
}

template <typename F, typename E>
auto Filter(F&& pred, const Event<E>& dep) -> Event<E>
    { return Filter(dep.GetGroup(), std::forward<F>(pred), dep); }

template <typename F, typename E, typename ... Ts>
auto Filter(const Group& group, F&& pred, const Event<E>& dep, const State<Ts>& ... states) -> Event<E>
{
    auto filterFunc = [capturedPred = std::forward<F>(pred)] (const EventValueList<E>& evts, EventValueSink<E> out, const Us& ... values)
    {
        for (const auto& v : evts)
            if (capturedPred(v, values ...))
                *out++ = v;
    };

    return Event<E>(group, std::move(filterFunc), dep, State ...);
}

template <typename F, typename E, typename ... Ts>
auto Filter(F&& pred, const Event<E>& dep, const State<Ts>& ... states) -> Event<E>
    { return Filter(dep.GetGroup(), std::forward<F>(pred), dep); }

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Transform
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E, typename F, typename T>
auto Transform(const Group& group, F&& op, const Event<T>& dep) -> Event<E>
{
    auto transformFunc = [capturedOp = std::forward<F>(op)] (EventRange<T> inRange, EventSink<E> out)
        { std::transform(inRange.begin(), inRange.end(), out, capturedOp); };

    return Event<E>(group, std::move(transformFunc), dep);
}

template <typename E, typename F, typename T>
auto Transform(F&& op, const Event<T>& dep) -> Event<E>
    { return Transform<E>(dep.GetGroup(), std::forward<F>(op), dep); }

template <typename E, typename F, typename T, typename ... Us>
auto Transform(const Group& group, F&& op, const Event<T>& dep, const State<Us>& ... states) -> Event<E>
{
    auto transformFunc = [capturedOp = std::forward<F>(pred)] (EventRange<T> inRange, EventSink<E> out, const Vs& ... values)
    {
        for (const auto& v : inRange)
            *out++ = capturedPred(v, values ...);
    };

    return Event<E>(group, std::move(transformFunc), dep, states ...);
}

template <typename E, typename F, typename T, typename ... Us>
auto Transform(F&& op, const Event<T>& dep, const State<Us>& ... states) -> Event<E>
    { return Transform<E>(dep.GetGroup(), std::forward<F>(op), dep, states ...); }

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Join
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename U1, typename ... Us>
auto Join(const Group& group, const Event<U1>& dep1, const Event<Us>& ... deps) -> Event<std::tuple<U1, Us ...>>
{
    using REACT_IMPL::EventJoinNode;

    static_assert(sizeof...(Us) > 0, "Join requires at least 2 inputs.");

    return Event<std::tuple<U1, Us ...>>::CreateWithNode<EventJoinNode<U1, Us ...>>(
        group, SameGroupOrLink(group, dep1), SameGroupOrLink(group, deps) ...);
}

template <typename U1, typename ... Us>
auto Join(const Event<U1>& dep1, const Event<Us>& ... deps) -> Event<std::tuple<U1, Us ...>>
    { return Join(dep1.GetGroup(), dep1, deps ...); }

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Token
///////////////////////////////////////////////////////////////////////////////////////////////////
enum class Token { value };

/*struct Tokenizer
{
    template <typename T>
    Token operator()(const T&) const { return Token::value; }
};

template <typename T>
auto Tokenize(T&& source) -> decltype(auto)
{
    return Transform(source, Tokenizer{ });
}*/

/******************************************/ REACT_END /******************************************/

/***************************************/ REACT_IMPL_BEGIN /**************************************/

template <typename E>
static Event<E> SameGroupOrLink(const Group& targetGroup, const Event<E>& dep)
{
    if (dep.GetGroup() == targetGroup)
        return dep;
    else
        return EventLink<E>{ targetGroup, dep };
}

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_EVENT_H_INCLUDED