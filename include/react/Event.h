
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

struct PrivateEventLinkNodeInterface;

/****************************************/ REACT_IMPL_END /***************************************/

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Event
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E>
class Event
{
private:
    using NodeType = REACT_IMPL::EventStreamNode<E>;

public:
    Event() = default;

    Event(const Event&) = default;
    Event& operator=(const Event&) = default;

    Event(Event&&) = default;
    Event& operator=(Event&&) = default;

    template <typename F, typename T>
    Event(F&& func, const Event<T>& dep) :
        Event::Event( REACT_IMPL::CtorTag{ }, CreateProcessingNode(std::forward<F>(func), dep) )
        { }

    template <typename F, typename T>
    Event(const ReactiveGroup& group, F&& func, const Event<T>& dep) :
        Event::Event( REACT_IMPL::CtorTag{ }, CreateProcessingNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group), std::forward<F>(func), dep) )
        { }

    template <typename F, typename T, typename ... Us>
    Event(F&& func, const Event<T>& dep, const Signal<Us>& ... signals) :
        Event::Event( REACT_IMPL::CtorTag{ }, CreateSyncedProcessingNode(std::forward<F>(func), dep, signals ...) )
        { }

    template <typename F, typename T, typename ... Us>
    Event(const ReactiveGroup& group, F&& func, const Event<T>& dep, const Signal<Us>& ... signals) :
        Event::Event( REACT_IMPL::CtorTag{ }, CreateSyncedProcessingNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group), std::forward<F>(func), dep, signals ...) )
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

protected:
    // Internal node ctor
    Event(REACT_IMPL::CtorTag, std::shared_ptr<NodeType>&& nodePtr) :
        nodePtr_( std::move(nodePtr) )
        { }

    auto NodePtr() -> std::shared_ptr<NodeType>&
        { return nodePtr_; }

    auto NodePtr() const -> const std::shared_ptr<NodeType>&
        { return nodePtr_; }

    template <typename F, typename T>
    auto CreateProcessingNode(F&& func, const Event<T>& dep) -> decltype(auto)
    {
        using REACT_IMPL::PrivateNodeInterface;
        using EventNodeType = REACT_IMPL::EventProcessingNode<E, T, typename std::decay<F>::type>;

        return std::make_shared<EventNodeType>(PrivateNodeInterface::GraphPtr(dep), std::forward<F>(func), PrivateNodeInterface::NodePtr(dep));
    }

    template <typename F, typename T, typename ... Us>
    auto CreateSyncedProcessingNode(F&& func, const Event<T>& dep, const Signal<Us>& ... syncs) -> decltype(auto)
    {
        using REACT_IMPL::GetCheckedGraphPtr;
        using REACT_IMPL::PrivateNodeInterface;
        using EventNodeType = REACT_IMPL::SyncedEventProcessingNode<E, T, typename std::decay<F>::type, Us ...>;

        return std::make_shared<EventNodeType>(
            GetCheckedGraphPtr(dep, syncs ...),
            std::forward<F>(func),
            PrivateNodeInterface::NodePtr(dep), PrivateNodeInterface::NodePtr(syncs) ...);
    }

private:
    std::shared_ptr<NodeType> nodePtr_;

    friend struct REACT_IMPL::PrivateNodeInterface;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventSource
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E>
class EventSource : public Event<E>
{
public:
    EventSource() = default;

    EventSource(const EventSource&) = default;
    EventSource& operator=(const EventSource&) = default;

    EventSource(EventSource&& other) = default;
    EventSource& operator=(EventSource&& other) = default;

    // Construct event source
    explicit EventSource(const ReactiveGroup& group) :
        EventSource::Event( REACT_IMPL::CtorTag{ }, CreateSourceNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group)) )
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


    auto CreateSourceNode(const std::shared_ptr<REACT_IMPL::ReactiveGraph>& graphPtr) -> decltype(auto)
    {
        using SrcNodeType = REACT_IMPL::EventSourceNode<E>;
        return std::make_shared<SrcNodeType>(graphPtr);
    }

private:
    template <typename T>
    void EmitValue(T&& value)
    {
        using REACT_IMPL::NodeId;
        using REACT_IMPL::ReactiveGraph;
        using SrcNodeType = REACT_IMPL::EventSourceNode<E>;

        SrcNodeType* castedPtr = static_cast<SrcNodeType*>(this->NodePtr().get());

        NodeId nodeId = castedPtr->GetNodeId();
        auto& graphPtr = NodePtr()->GraphPtr();
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
    EventSlot() = default;

    EventSlot(const EventSlot&) = default;
    EventSlot& operator=(const EventSlot&) = default;

    EventSlot(EventSlot&&) = default;
    EventSlot& operator=(EventSlot&&) = default;

    // Construct with value
    EventSlot(const ReactiveGroup& group, const Event<E>& input) :
        EventSlot::EventSlotBase( REACT_IMPL::CtorTag{ }, CreateSlotNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group), input) )
        { }

    void Set(const Event<E>& newInput)
        { SetInput(newInput); }

    void operator<<=(const Event<E>& newInput)
        { SetInput(newInput); }

protected:
    auto CreateSlotNode(const std::shared_ptr<REACT_IMPL::ReactiveGraph>& graphPtr, const Event<E>& input) -> decltype(auto)
    {
        using REACT_IMPL::PrivateNodeInterface;
        using REACT_IMPL::PrivateReactiveGroupInterface;
        using SlotNodeType = REACT_IMPL::EventSlotNode<E>;

        return std::make_shared<SlotNodeType>(PrivateReactiveGroupInterface::GraphPtr(group), PrivateNodeInterface::NodePtr(input));
    }

private:
    void SetInput(const Event<E>& newInput)
    {
        using REACT_IMPL::PrivateNodeInterface;
        using REACT_IMPL::NodeId;
        using REACT_IMPL::ReactiveGraph;
        using SlotNodeType = REACT_IMPL::EventSlotNode<E>;

        SlotNodeType* castedPtr = static_cast<SlotNodeType*>(this->NodePtr().get());

        NodeId nodeId = castedPtr->GetInputNodeId();
        auto& graphPtr = NodePtr()->GraphPtr();
        graphPtr->AddInput(nodeId, [castedPtr, &newInput] { castedPtr->SetInput(PrivateNodeInterface::NodePtr(newInput)); });
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventLink
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E>
class EventLink : public Event<E>
{
public:
    EventLink() = default;

    EventLink(const EventLink&) = default;
    EventLink& operator=(const EventLink&) = default;

    EventLink(EventLink&&) = default;
    EventLink& operator=(EventLink&&) = default;

    // Construct with explicit group
    EventLink(const ReactiveGroup& group, const Event<E>& input) :
        SignalLink::Signal( REACT_IMPL::CtorTag{ }, CreateLinkNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group), input) )
        { }

    // Construct with implicit group
    explicit EventLink(const Event<E>& input) :
        SignalLink::Signal( REACT_IMPL::CtorTag{ }, CreateLinkNode(REACT_IMPL::PrivateNodeInterface::GraphPtr(input), input) )
        { }

protected:
    static auto CreateLinkNode(const std::shared_ptr<REACT_IMPL::ReactiveGraph>& graphPtr, const Event<E>& input) -> decltype(auto)
    {
        using REACT_IMPL::PrivateNodeInterface;
        using REACT_IMPL::PrivateReactiveGroupInterface;
        using EventNodeType = REACT_IMPL::EventLinkNode<E>;

        auto node = std::make_shared<EventNodeType>(graphPtr, PrivateNodeInterface::GraphPtr(input), PrivateNodeInterface::NodePtr(input));
        node->SetWeakSelfPtr(std::weak_ptr<EventNodeType>{ node });
        return node;
    }

    friend struct REACT_IMPL::PrivateEventLinkNodeInterface;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Merge
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T = void, typename U1, typename ... Us>
auto Merge(const ReactiveGroup& group, const Event<U1>& dep1, const Event<Us>& ... deps) -> decltype(auto)
{
    using REACT_IMPL::EventMergeNode;
    using REACT_IMPL::PrivateEventLinkNodeInterface;
    using REACT_IMPL::PrivateReactiveGroupInterface;
    using REACT_IMPL::PrivateNodeInterface;
    using REACT_IMPL::CtorTag;

    static_assert(sizeof...(Us) > 0, "Merge requires at least 2 inputs.");

    // If supplied, use merge type, otherwise default to common type.
    using E = typename std::conditional<
        std::is_same<T, void>::value,
        typename std::common_type<U1, Us ...>::type,
        T>::type;

    const auto& graphPtr = PrivateReactiveGroupInterface::GraphPtr(group);
    
    return PrivateNodeInterface::CreateNodeHelper<Event<E>, EventMergeNode<E, U1, Us ...>>(
        graphPtr, PrivateEventLinkNodeInterface::GetLocalNodePtr(graphPtr, dep1), PrivateEventLinkNodeInterface::GetLocalNodePtr(graphPtr, deps) ...);
}

template <typename T = void, typename U1, typename ... Us>
auto Merge(const Event<U1>& dep1, const Event<Us>& ... deps) -> decltype(auto)
{
    using REACT_IMPL::EventMergeNode;
    using REACT_IMPL::PrivateEventLinkNodeInterface;
    using REACT_IMPL::PrivateNodeInterface;
    using REACT_IMPL::CtorTag;

    static_assert(sizeof...(Us) > 0, "Merge requires at least 2 inputs.");

    // If supplied, use merge type, otherwise default to common type.
    using E = typename std::conditional<
        std::is_same<T, void>::value,
        typename std::common_type<U1, Us ...>::type,
        T>::type;

    const auto& graphPtr = PrivateNodeInterface::GraphPtr(dep1);

    return PrivateNodeInterface::CreateNodeHelper<Event<E>, EventMergeNode<E, U1, Us ...>>(
        graphPtr, PrivateEventLinkNodeInterface::GetLocalNodePtr(graphPtr, dep1), PrivateEventLinkNodeInterface::GetLocalNodePtr(graphPtr, deps) ...);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Filter
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename F, typename E>
auto Filter(const ReactiveGroup& group, F&& pred, const Event<E>& dep) -> Event<E>
{
    auto filterFunc = [capturedPred = std::forward<F>(pred)] (EventRange<E> inRange, EventSink<E> out)
        { std::copy_if(inRange.begin(), inRange.end(), out, capturedPred); };

    return Event<E>(group, std::move(filterFunc), dep);
}

template <typename F, typename E>
auto Filter(F&& pred, const Event<E>& dep) -> Event<E>
{
    auto filterFunc = [capturedPred = std::forward<F>(pred)] (EventRange<E> inRange, EventSink<E> out)
        { std::copy_if(inRange.begin(), inRange.end(), out, capturedPred); };

    return Event<E>(std::move(filterFunc), dep);
}

template <typename F, typename E, typename ... Ts>
auto Filter(const ReactiveGroup& group, F&& pred, const Event<E>& dep, const Signal<Ts>& ... signals) -> Event<E>
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
{
    auto filterFunc = [capturedPred = std::forward<F>(pred)] (EventRange<E> inRange, EventSink<E> out, const Us& ... values)
    {
        for (const auto& v : inRange)
            if (capturedPred(v, values ...))
                *out++ = v;
    };

    return Event<E>(std::move(filterFunc), dep, signals ...);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Transform
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E, typename F, typename T>
auto Transform(const ReactiveGroup& group, F&& op, const Event<T>& dep) -> Event<E>
{
    auto transformFunc = [capturedOp = std::forward<F>(op)] (EventRange<T> inRange, EventSink<E> out)
        { std::transform(inRange.begin(), inRange.end(), out, capturedOp); };

    return Event<E>(group, std::move(transformFunc), dep);
}

template <typename E, typename F, typename T>
auto Transform(F&& op, const Event<T>& dep) -> Event<E>
{
    auto transformFunc = [capturedOp = std::forward<F>(op)] (EventRange<T> inRange, EventSink<E> out)
        { std::transform(inRange.begin(), inRange.end(), out, capturedOp); };

    return Event<E>(std::move(transformFunc), dep);
}

template <typename E, typename F, typename T, typename ... Us>
auto Transform(const ReactiveGroup& group, F&& op, const Event<T>& dep, const Signal<Us>& ... signals) -> Event<E>
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
{
    auto transformFunc = [capturedOp = std::forward<F>(pred)] (EventRange<T> inRange, EventSink<E> out, const Vs& ... values)
    {
        for (const auto& v : inRange)
            *out++ = capturedPred(v, values ...);
    };

    return Event<E>(std::move(transformFunc), dep, signals ...);
}

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
auto Join(const ReactiveGroup& group, const Event<U1>& dep1, const Event<Us>& ... deps) -> Event<std::tuple<U1, Us ...>>
{
    using REACT_IMPL::EventJoinNode;
    using REACT_IMPL::PrivateReactiveGroupInterface;
    using REACT_IMPL::PrivateEventLinkNodeInterface;
    using REACT_IMPL::PrivateNodeInterface;

    static_assert(sizeof...(Us) > 0, "Join requires at least 2 inputs.");

    // If supplied, use merge type, otherwise default to common type.
    const auto& graphPtr = PrivateReactiveGroupInterface::GraphPtr(group);

    return PrivateNodeInterface::CreateNodeHelper<Event<std::tuple<U1, Us ...>>, EventJoinNode<U1, Us ...>>(
        graphPtr, PrivateEventLinkNodeInterface::GetLocalNodePtr(graphPtr, dep1), PrivateEventLinkNodeInterface::GetLocalNodePtr(graphPtr, deps) ...);
}

template <typename U1, typename ... Us>
auto Join(const Event<U1>& dep1, const Event<Us>& ... deps) -> Event<std::tuple<U1, Us ...>>
{
    using REACT_IMPL::EventJoinNode;
    using REACT_IMPL::PrivateEventLinkNodeInterface;
    using REACT_IMPL::PrivateNodeInterface;

    static_assert(sizeof...(Us) > 0, "Join requires at least 2 inputs.");

    // If supplied, use merge type, otherwise default to common type.
    const auto& graphPtr = PrivateNodeInterface::GraphPtr(dep1);

    return PrivateNodeInterface::CreateNodeHelper<Event<std::tuple<U1, Us ...>>, EventJoinNode<U1, Us ...>>(
        graphPtr, PrivateEventLinkNodeInterface::GetLocalNodePtr(graphPtr, dep1), PrivateEventLinkNodeInterface::GetLocalNodePtr(graphPtr, deps) ...);
}

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

template <typename L, typename R>
bool Equals(const Event<L>& lhs, const Event<R>& rhs)
{
    return lhs.Equals(rhs);
}

struct PrivateEventLinkNodeInterface
{
    template <typename E>
    static auto GetLocalNodePtr(const std::shared_ptr<ReactiveGraph>& targetGraph, const Event<E>& dep) -> std::shared_ptr<EventStreamNode<E>>
    {
        const std::shared_ptr<ReactiveGraph>& sourceGraph = PrivateNodeInterface::GraphPtr(dep);

        if (sourceGraph == targetGraph)
        {
            return PrivateNodeInterface::NodePtr(dep);
        }
        else
        {
            return EventLink<E>::CreateLinkNode(targetGraph, dep);
        }
    }
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_EVENT_H_INCLUDED