
//          Copyright Sebastian Jeckel 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_EVENT_H_INCLUDED
#define REACT_EVENT_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"
#include "react/API.h"
#include "react/Group.h"

#include <memory>
#include <type_traits>
#include <utility>

#include "react/detail/graph/EventNodes.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

template <typename E>
class EventInternals
{
protected:
    using NodeType = EventStreamNode<E>;
    using StorageType = typename NodeType::StorageType;

public:
    EventInternals(const EventInternals&) = default;
    EventInternals& operator=(const EventInternals&) = default;

    EventInternals(EventInternals&&) = default;
    EventInternals& operator=(EventInternals&&) = default;

    EventInternals(std::shared_ptr<NodeType>&& nodePtr) :
        nodePtr_( std::move(nodePtr) )
        { }

    auto GetNodePtr() -> std::shared_ptr<NodeType>&
        { return nodePtr_; }

    auto GetNodePtr() const -> const std::shared_ptr<NodeType>&
        { return nodePtr_; }

    NodeId GetNodeId() const
        { return nodePtr_->GetNodeId(); }

    StorageType& Events()
        { return nodePtr_->Events(); }

    const StorageType& Events() const
        { return nodePtr_->Events(); }

    void SetPendingSuccessorCount(size_t count)
        { nodePtr_->SetPendingSuccessorCount(count); }

    void DecrementPendingSuccessorCount()
        { nodePtr_->DecrementPendingSuccessorCount(); }

private:
    std::shared_ptr<NodeType> nodePtr_;
};

/****************************************/ REACT_IMPL_END /***************************************/

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

    template <typename F, typename T>
    Event(F&& func, const Event<T>& dep) :
        Event::Event( REACT_IMPL::CtorTag{ }, CreateProcessingNode(dep.GetGroup(), std::forward<F>(func), dep) )
        { }

    template <typename F, typename T>
    Event(const Group& group, F&& func, const Event<T>& dep) :
        Event::Event( REACT_IMPL::CtorTag{ }, CreateProcessingNode(group, std::forward<F>(func), dep) )
        { }

    template <typename F, typename T, typename ... Us>
    Event(F&& func, const Event<T>& dep, const Signal<Us>& ... signals) :
        Event::Event( REACT_IMPL::CtorTag{ }, CreateSyncedProcessingNode(dep.GetGroup(), std::forward<F>(func), dep, signals ...) )
        { }

    template <typename F, typename T, typename ... Us>
    Event(const Group& group, F&& func, const Event<T>& dep, const Signal<Us>& ... signals) :
        Event::Event( REACT_IMPL::CtorTag{ }, CreateSyncedProcessingNode(group, std::forward<F>(func), dep, signals ...) )
        { }

    auto Tokenize() const -> decltype(auto)
        { return REACT::Tokenize(*this); }

    /*template <typename ... Us>
    auto Merge(Us&& ... deps) const -> decltype(auto)
        { return REACT::Merge(*this, std::forward<Us>(deps) ...); }

    template <typename F>
    auto Filter(F&& pred) const -> decltype(auto)
        { return REACT::Filter(std::forward<F>(pred), *this); }

    template <typename F>
    auto Transform(F&& pred) const -> decltype(auto)
        { return REACT::Transform(*this, std::forward<F>(f)); }*/

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

public: // Internal
    template <typename TNode, typename ... TArgs>
    static Event<E> CreateWithNode(TArgs&& ... args)
    {
        return Event<E>( REACT_IMPL::CtorTag{ }, std::make_shared<TNode>(std::forward<TArgs>(args) ...) );
    }

protected:
    // Private node ctor
    Event(REACT_IMPL::CtorTag, std::shared_ptr<REACT_IMPL::EventStreamNode<E>>&& nodePtr) :
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
    auto CreateSyncedProcessingNode(const Group& group, F&& func, const Event<T>& dep, const Signal<Us>& ... syncs) -> decltype(auto)
    {
        using REACT_IMPL::SyncedEventProcessingNode;
        using REACT_IMPL::SameGroupOrLink;

        return std::make_shared<SyncedEventProcessingNode<E, T, typename std::decay<F>::type, Us ...>>(
            group, std::forward<F>(func), SameGroupOrLink(group, dep), SameGroupOrLink(group, syncs) ...);
    }
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
        EventSource::Event( REACT_IMPL::CtorTag{ }, CreateSourceNode(group) )
        { }
    
    void Emit(const E& value)
        { EmitValue(value); }

    void Emit(E&& value)
        { EmitValue(std::move(value)); }

    template <typename T = E, typename = typename std::enable_if<std::is_same<T,Token>::value>::type>
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
        using REACT_IMPL::ReactiveGraph;
        using SrcNodeType = REACT_IMPL::EventSourceNode<E>;

        SrcNodeType* castedPtr = static_cast<SrcNodeType*>(this->GetNodePtr().get());

        NodeId nodeId = castedPtr->GetNodeId();
        auto& graphPtr = GetInternals(this->GetGroup()).GetGraphPtr();

        graphPtr->AddInput(nodeId, [castedPtr, &value] { castedPtr->EmitValue(std::forward<T>(value)); });
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
        EventSlot::Event( REACT_IMPL::CtorTag{ }, CreateSlotNode(group) )
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

        graphPtr->AddInput(nodeId, [this, castedPtr, &input] { castedPtr->AddInput(SameGroupOrLink(GetGroup(), input)); });
    }

    void RemoveInput(const Event<E>& input)
    {
        using REACT_IMPL::NodeId;
        using SlotNodeType = REACT_IMPL::EventSlotNode<E>;

        SlotNodeType* castedPtr = static_cast<SlotNodeType*>(this->GetNodePtr().get());

        NodeId nodeId = castedPtr->GetInputNodeId();
        auto& graphPtr = GetInternals(this->GetGroup()).GetGraphPtr();

        graphPtr->AddInput(nodeId, [this, castedPtr, &input] { castedPtr->RemoveInput(SameGroupOrLink(GetGroup(), input)); });
    }

    void RemoveAllInputs()
    {
        using REACT_IMPL::NodeId;
        using SlotNodeType = REACT_IMPL::EventSlotNode<E>;

        SlotNodeType* castedPtr = static_cast<SlotNodeType*>(this->GetNodePtr().get());

        NodeId nodeId = castedPtr->GetInputNodeId();
        auto& graphPtr = GetInternals(this->GetGroup()).GetGraphPtr();

        graphPtr->AddInput(nodeId, [castedPtr] { castedPtr->RemoveAllInputs(); });
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
        EventLink::Event( REACT_IMPL::CtorTag{ }, GetOrCreateLinkNode(group, input) )
        { }

protected:
    static auto GetOrCreateLinkNode(const Group& group, const Event<E>& input) -> decltype(auto)
    {
        using REACT_IMPL::EventLinkNode;

        auto& targetGraphPtr = GetInternals(group).GetGraphPtr();
        auto& linkCache = targetGraphPtr->GetLinkCache();
        
        void* k1 = GetInternals(input.GetGroup()).GetGraphPtr().get();
        void* k2 = GetInternals(input).GetNodePtr().get();

        auto nodePtr = linkCache.LookupOrCreate<EventLinkNode<E>>(
            { k1, k2 },
            [&]
            {
                auto nodePtr = std::make_shared<EventLinkNode<E>>(group, input);
                nodePtr->SetWeakSelfPtr(std::weak_ptr<EventLinkNode<E>>{ nodePtr });
                return nodePtr;
            });

        return nodePtr;
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Merge
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T = void, typename U1, typename ... Us>
auto Merge(const Group& group, const Event<U1>& dep1, const Event<Us>& ... deps) -> decltype(auto)
{
    using REACT_IMPL::EventMergeNode;
    using REACT_IMPL::SameGroupOrLink;
    using REACT_IMPL::CtorTag;

    static_assert(sizeof...(Us) > 0, "Merge requires at least 2 inputs.");

    // If supplied, use merge type, otherwise default to common type.
    using E = typename std::conditional<
        std::is_same<T, void>::value,
        typename std::common_type<U1, Us ...>::type,
        T>::type;

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
    auto filterFunc = [capturedPred = std::forward<F>(pred)] (EventRange<E> inRange, EventSink<E> out)
        { std::copy_if(inRange.begin(), inRange.end(), out, capturedPred); };

    return Event<E>(group, std::move(filterFunc), dep);
}

template <typename F, typename E>
auto Filter(F&& pred, const Event<E>& dep) -> Event<E>
    { return Filter(dep.GetGroup(), std::forward<F>(pred), dep); }

template <typename F, typename E, typename ... Ts>
auto Filter(const Group& group, F&& pred, const Event<E>& dep, const Signal<Ts>& ... signals) -> Event<E>
{
    auto filterFunc = [capturedPred = std::forward<F>(pred)] (EventRange<E> inRange, EventSink<E> out, const Us& ... values)
    {
        for (const auto& v : inRange)
            if (capturedPred(v, values ...))
                *out++ = v;
    };

    return Event<E>(group, std::move(filterFunc), dep, signals ...);
}

template <typename F, typename E, typename ... Ts>
auto Filter(F&& pred, const Event<E>& dep, const Signal<Ts>& ... signals) -> Event<E>
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
auto Transform(const Group& group, F&& op, const Event<T>& dep, const Signal<Us>& ... signals) -> Event<E>
{
    auto transformFunc = [capturedOp = std::forward<F>(pred)] (EventRange<T> inRange, EventSink<E> out, const Vs& ... values)
    {
        for (const auto& v : inRange)
            *out++ = capturedPred(v, values ...);
    };

    return Event<E>(group, std::move(transformFunc), dep, signals ...);
}

template <typename E, typename F, typename T, typename ... Us>
auto Transform(F&& op, const Event<T>& dep, const Signal<Us>& ... signals) -> Event<E>
    { return Transform<E>(dep.GetGroup(), std::forward<F>(op), dep, signals ...); }

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Flatten
///////////////////////////////////////////////////////////////////////////////////////////////////
/*template <typename TInnerValue>
auto Flatten(const Signal<Events<TInnerValue>>& outer) -> Events<TInnerValue>
{
    return Events<TInnerValue>(
        std::make_shared<REACT_IMPL::EventFlattenNode<Events<TInnerValue>, TInnerValue>>(
            GetNodePtr(outer), GetNodePtr(outer.Value())));
}*/

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