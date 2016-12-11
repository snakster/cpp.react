
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
/// Events
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E = Token>
class EventBase
{
private:
    using NodeType = REACT_IMPL::EventStreamNode<E>;

public:
    // Private node ctor
    EventBase(REACT_IMPL::CtorTag, std::shared_ptr<NodeType>&& nodePtr) :
        nodePtr_( std::move(nodePtr) )
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
    EventBase() = default;

    EventBase(const EventBase&) = default;
    EventBase& operator=(const EventBase&) = default;

    EventBase(EventBase&&) = default;
    EventBase& operator=(EventBase&&) = default;

    auto NodePtr() -> std::shared_ptr<NodeType>&
        { return nodePtr_; }

    auto NodePtr() const -> const std::shared_ptr<NodeType>&
        { return nodePtr_; }

    template <typename F, typename T>
    auto CreateProcessingNode(F&& func, const EventBase<T>& dep) -> decltype(auto)
    {
        using REACT_IMPL::PrivateNodeInterface;
        using EventNodeType = REACT_IMPL::EventProcessingNode<E, T, typename std::decay<F>::type>;

        return std::make_shared<EventNodeType>(PrivateNodeInterface::GraphPtr(dep), std::forward<F>(func), PrivateNodeInterface::NodePtr(dep));
    }

    template <typename F, typename T, typename ... Us>
    auto CreateSyncedProcessingNode(F&& func, const EventBase<T>& dep, const SignalBase<Us>& ... syncs) -> decltype(auto)
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
template <typename E = Token>
class EventSourceBase : public EventBase<E>
{
public:
    using EventBase::EventBase;
    
    void Emit(const E& value)
        { EmitValue(value); }

    void Emit(E&& value)
        { EmitValue(std::move(value)); }

    template <typename T = E, typename = typename std::enable_if<std::is_same<T,Token>::value>::type>
    void Emit()
        { EmitValue(Token::value); }

    EventSourceBase& operator<<(const E& value)
        { EmitValue(e); return *this; }

    EventSourceBase& operator<<(E&& value)
        { EmitValue(std::move(value)); return *this; }

protected:
    EventSourceBase() = default;

    EventSourceBase(const EventSourceBase&) = default;
    EventSourceBase& operator=(const EventSourceBase&) = default;

    EventSourceBase(EventSourceBase&& other) = default;
    EventSourceBase& operator=(EventSourceBase&& other) = default;

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
class EventSlotBase : public EventBase<E>
{
public:
    using EventBase::EventBase;

    void Set(const EventBase<E>& newInput)
        { SetInput(newInput); }

    void operator<<=(const EventBase<E>& newInput)
        { SetInput(newInput); }

protected:
    EventSlotBase() = default;

    EventSlotBase(const EventSlotBase&) = default;
    EventSlotBase& operator=(const EventSlotBase&) = default;

    EventSlotBase(EventSlotBase&&) = default;
    EventSlotBase& operator=(EventSlotBase&&) = default;

    auto CreateSlotNode(const std::shared_ptr<REACT_IMPL::ReactiveGraph>& graphPtr, const EventBase<E>& input) -> decltype(auto)
    {
        using REACT_IMPL::PrivateNodeInterface;
        using REACT_IMPL::PrivateReactiveGroupInterface;
        using SlotNodeType = REACT_IMPL::EventSlotNode<E>;

        return std::make_shared<SlotNodeType>(PrivateReactiveGroupInterface::GraphPtr(group), PrivateNodeInterface::NodePtr(input));
    }

private:
    void SetInput(const EventBase<E>& newInput)
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
/// EventLinkBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E>
class EventLinkBase : public EventBase<E>
{
public:
    using EventBase::EventBase;

protected:
    EventLinkBase() = default;

    EventLinkBase(const EventLinkBase&) = default;
    EventLinkBase& operator=(const EventLinkBase&) = default;

    EventLinkBase(EventLinkBase&&) = default;
    EventLinkBase& operator=(EventLinkBase&&) = default;

    static auto CreateLinkNode(const std::shared_ptr<REACT_IMPL::ReactiveGraph>& graphPtr, const EventBase<E>& input) -> decltype(auto)
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
/// Event
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E>
class Event<E, unique> : public EventBase<E>
{
public:
    using EventBase::EventBase;

    using ValueType = E;

    Event() = delete;

    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;

    Event(Event&&) = default;
    Event& operator=(Event&&) = default;

    template <typename F, typename T>
    Event(F&& func, const EventBase<T>& dep) :
        Event::EventBase( REACT_IMPL::CtorTag{ }, CreateProcessingNode(std::forward<F>(func), dep) )
        { }

    template <typename F, typename T>
    Event(const ReactiveGroupBase& group, F&& func, const EventBase<T>& dep) :
        Event::EventBase( REACT_IMPL::CtorTag{ }, CreateProcessingNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group), std::forward<F>(func), dep) )
        { }

    template <typename F, typename T, typename ... Us>
    Event(F&& func, const EventBase<T>& dep, const SignalBase<Us>& ... signals) :
        Event::EventBase( REACT_IMPL::CtorTag{ }, CreateSyncedProcessingNode(std::forward<F>(func), dep, signals ...) )
        { }

    template <typename F, typename T, typename ... Us>
    Event(const ReactiveGroupBase& group, F&& func, const EventBase<T>& dep, const SignalBase<Us>& ... signals) :
        Event::EventBase( REACT_IMPL::CtorTag{ }, CreateSyncedProcessingNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group), std::forward<F>(func), dep, signals ...) )
        { }
};

template <typename E>
class Event<E, shared> : public EventBase<E>
{
public:
    using EventBase::EventBase;

    using ValueType = E;

    Event() = delete;

    Event(const Event&) = default;
    Event& operator=(const Event&) = default;

    Event(Event&&) = default;
    Event& operator=(Event&&) = default;

    Event(Event<E, unique>&& other) :
        Event::EventBase( std::move(other) )
        { }

    Event& operator=(Event<E, unique>&& other)
        { Event::EventBase::operator=(std::move(other)); return *this; }

    template <typename F, typename T>
    Event(F&& func, const EventBase<T>& dep) :
        Event::EventBase( REACT_IMPL::CtorTag{ }, CreateProcessingNode(std::forward<F>(func), dep) )
        { }

    template <typename F, typename T>
    Event(const ReactiveGroupBase& group, F&& func, const EventBase<T>& dep) :
        Event::EventBase( REACT_IMPL::CtorTag{ }, CreateProcessingNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group), std::forward<F>(func), dep) )
        { }

    template <typename F, typename T, typename ... Us>
    Event(F&& func, const EventBase<T>& dep, const SignalBase<Us>& ... signals) :
        Event::EventBase( REACT_IMPL::CtorTag{ }, CreateSyncedProcessingNode(std::forward<F>(func), dep, signals ...) )
        { }

    template <typename F, typename T, typename ... Us>
    Event(const ReactiveGroupBase& group, F&& func, const EventBase<T>& dep, const SignalBase<Us>& ... signals) :
        Event::EventBase( REACT_IMPL::CtorTag{ }, CreateSyncedProcessingNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group), std::forward<F>(func), dep, signals ...) )
        { }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventSource
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E>
class EventSource<E, unique> : public EventSourceBase<E>
{
public:
    using EventSourceBase::EventSourceBase;

    using ValueType = E;

    EventSource() = delete;

    EventSource(const EventSource&) = delete;
    EventSource& operator=(const EventSource&) = delete;

    EventSource(EventSource&&) = default;
    EventSource& operator=(EventSource&&) = default;

    // Construct event source
    explicit EventSource(const ReactiveGroupBase& group) :
        EventSource::EventSourceBase( REACT_IMPL::CtorTag{ }, CreateSourceNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group)) )
        { }
};

template <typename E>
class EventSource<E, shared> : public EventSourceBase<E>
{
public:
    using EventSourceBase::EventSourceBase;

    using ValueType = E;

    EventSource() = delete;

    EventSource(const EventSource&) = default;
    EventSource& operator=(const EventSource&) = default;

    EventSource(EventSource&&) = default;
    EventSource& operator=(EventSource&&) = default;

    // Construct event source
    explicit EventSource(const ReactiveGroupBase& group) :
        EventSource::EventSourceBase( REACT_IMPL::CtorTag{ }, CreateSourceNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group)) )
        { }

    // Construct from unique
    EventSource(EventSource<E, unique>&& other) :
        EventSource::EventSourceBase( std::move(other) )
        { }

    // Assign from unique
    EventSource& operator=(EventSource<E, unique>&& other)
        { EventSource::EventSourceBase::operator=(std::move(other)); return *this; }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventSlot
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E>
class EventSlot<E, unique> : public EventSlotBase<E>
{
public:
    using EventSlotBase::EventSlotBase;

    using ValueType = E;

    EventSlot() = delete;

    EventSlot(const EventSlot&) = delete;
    EventSlot& operator=(const EventSlot&) = delete;

    EventSlot(EventSlot&&) = default;
    EventSlot& operator=(EventSlot&&) = default;

    // Construct with value
    EventSlot(const ReactiveGroupBase& group, const EventBase<E>& input) :
        EventSlot::EventSlotBase( REACT_IMPL::CtorTag{ }, CreateSlotNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group), input) )
        { }
};

template <typename E>
class EventSlot<E, shared> : public EventSlotBase<E>
{
public:
    using EventSlotBase::EventSlotBase;

    using ValueType = E;

    EventSlot() = delete;

    EventSlot(const EventSlot&) = default;
    EventSlot& operator=(const EventSlot&) = default;

    EventSlot(EventSlot&&) = default;
    EventSlot& operator=(EventSlot&&) = default;

    // Construct from unique
    EventSlot(EventSlot<E, unique>&& other) :
        EventSlot::EventSlotBase( std::move(other) )
        { }

    // Assign from unique
    EventSlot& operator=(EventSlot<E, unique>&& other)
        { EventSlot::EventSlotBase::operator=(std::move(other)); return *this; }

    // Construct with value
    EventSlot(const ReactiveGroupBase& group, const SignalBase<E>& input) :
        EventSlot::EventSlotBase( REACT_IMPL::CtorTag{ }, CreateSlotNode(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group), input) )
        { }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Merge
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T = void, typename U1, typename ... Us>
auto Merge(const ReactiveGroupBase& group, const EventBase<U1>& dep1, const EventBase<Us>& ... deps) -> decltype(auto)
{
    using REACT_IMPL::EventMergeNode;
    using REACT_IMPL::PrivateEventLinkNodeInterface;
    using REACT_IMPL::PrivateReactiveGroupInterface;
    using REACT_IMPL::CtorTag;

    static_assert(sizeof...(Us) > 0, "Merge requires at least 2 inputs.");

    // If supplied, use merge type, otherwise default to common type.
    using E = typename std::conditional<
        std::is_same<T, void>::value,
        typename std::common_type<U1, Us ...>::type,
        T>::type;

    const auto& graphPtr = PrivateReactiveGroupInterface::GraphPtr(group);

    return Event<E, unique>( CtorTag{ }, std::make_shared<EventMergeNode<E, U1, Us ...>>(
        graphPtr, PrivateEventLinkNodeInterface::GetLocalNodePtr(graphPtr, dep1), PrivateEventLinkNodeInterface::GetLocalNodePtr(graphPtr, deps) ...));
}

template <typename T = void, typename U1, typename ... Us>
auto Merge(const EventBase<U1>& dep1, const EventBase<Us>& ... deps) -> decltype(auto)
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

    return Event<E, unique>( CtorTag{ }, std::make_shared<EventMergeNode<E, U1, Us ...>>(
        graphPtr, PrivateEventLinkNodeInterface::GetLocalNodePtr(graphPtr, dep1), PrivateEventLinkNodeInterface::GetLocalNodePtr(graphPtr, deps) ...));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Filter
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename F, typename E>
auto Filter(const ReactiveGroupBase& group, F&& pred, const EventBase<E>& dep) -> Event<E, unique>
{
    auto filterFunc = [capturedPred = std::forward<F>(pred)] (EventRange<E> inRange, EventSink<E> out)
        { std::copy_if(inRange.begin(), inRange.end(), out, capturedPred); };

    return Event<E, unique>(group, std::move(filterFunc), dep);
}

template <typename F, typename E>
auto Filter(F&& pred, const EventBase<E>& dep) -> Event<E, unique>
{
    auto filterFunc = [capturedPred = std::forward<F>(pred)] (EventRange<E> inRange, EventSink<E> out)
        { std::copy_if(inRange.begin(), inRange.end(), out, capturedPred); };

    return Event<E, unique>(std::move(filterFunc), dep);
}

template <typename F, typename E, typename ... Ts>
auto Filter(const ReactiveGroupBase& group, F&& pred, const EventBase<E>& dep, const SignalBase<Ts>& ... signals) -> Event<E, unique>
{
    auto filterFunc = [capturedPred = std::forward<F>(pred)] (EventRange<E> inRange, EventSink<E> out, const Us& ... values)
    {
        for (const auto& v : inRange)
            if (capturedPred(v, values ...))
                *out++ = v;
    };

    return Event<E, unique>(group, std::move(filterFunc), dep, signals ...);
}

template <typename F, typename E, typename ... Ts>
auto Filter(F&& pred, const EventBase<E>& dep, const SignalBase<Ts>& ... signals) -> Event<E, unique>
{
    auto filterFunc = [capturedPred = std::forward<F>(pred)] (EventRange<E> inRange, EventSink<E> out, const Us& ... values)
    {
        for (const auto& v : inRange)
            if (capturedPred(v, values ...))
                *out++ = v;
    };

    return Event<E, unique>(std::move(filterFunc), dep, signals ...);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Transform
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename E, typename F, typename T>
auto Transform(const ReactiveGroupBase& group, F&& op, const EventBase<T>& dep) -> Event<E, unique>
{
    auto transformFunc = [capturedOp = std::forward<F>(op)] (EventRange<T> inRange, EventSink<E> out)
        { std::transform(inRange.begin(), inRange.end(), out, capturedOp); };

    return Event<E, unique>(group, std::move(transformFunc), dep);
}

template <typename E, typename F, typename T>
auto Transform(F&& op, const EventBase<T>& dep) -> Event<E, unique>
{
    auto transformFunc = [capturedOp = std::forward<F>(op)] (EventRange<T> inRange, EventSink<E> out)
        { std::transform(inRange.begin(), inRange.end(), out, capturedOp); };

    return Event<E, unique>(std::move(transformFunc), dep);
}

template <typename E, typename F, typename T, typename ... Us>
auto Transform(const ReactiveGroupBase& group, F&& op, const EventBase<T>& dep, const SignalBase<Us>& ... signals) -> Event<E, unique>
{
    auto transformFunc = [capturedOp = std::forward<F>(pred)] (EventRange<T> inRange, EventSink<E> out, const Vs& ... values)
    {
        for (const auto& v : inRange)
            *out++ = capturedPred(v, values ...);
    };

    return Event<E, unique>(group, std::move(transformFunc), dep, signals ...);
}

template <typename E, typename F, typename T, typename ... Us>
auto Transform(F&& op, const EventBase<T>& dep, const SignalBase<Us>& ... signals) -> Event<E, unique>
{
    auto transformFunc = [capturedOp = std::forward<F>(pred)] (EventRange<T> inRange, EventSink<E> out, const Vs& ... values)
    {
        for (const auto& v : inRange)
            *out++ = capturedPred(v, values ...);
    };

    return Event<E, unique>(std::move(transformFunc), dep, signals ...);
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
auto Join(const ReactiveGroupBase& group, const EventBase<U1>& dep1, const EventBase<Us>& ... deps) -> Event<std::tuple<U1, Us ...>, unique>
{
    using REACT_IMPL::EventJoinNode;
    using REACT_IMPL::PrivateReactiveGroupInterface;
    using REACT_IMPL::PrivateEventLinkNodeInterface;
    using REACT_IMPL::CtorTag;

    static_assert(sizeof...(Us) > 0, "Join requires at least 2 inputs.");

    // If supplied, use merge type, otherwise default to common type.
    const auto& graphPtr = PrivateReactiveGroupInterface::GraphPtr(group);

    return Event<std::tuple<U1, Us ...>, unique>( CtorTag{ }, std::make_shared<EventJoinNode<U1, Us ...>>(
        graphPtr, PrivateEventLinkNodeInterface::GetLocalNodePtr(graphPtr, dep1), PrivateEventLinkNodeInterface::GetLocalNodePtr(graphPtr, deps) ...));
}

template <typename U1, typename ... Us>
auto Join(const EventBase<U1>& dep1, const EventBase<Us>& ... deps) -> Event<std::tuple<U1, Us ...>, unique>
{
    using REACT_IMPL::EventJoinNode;
    using REACT_IMPL::PrivateNodeInterface;
    using REACT_IMPL::PrivateEventLinkNodeInterface;
    using REACT_IMPL::CtorTag;

    static_assert(sizeof...(Us) > 0, "Join requires at least 2 inputs.");

    // If supplied, use merge type, otherwise default to common type.
    const auto& graphPtr = PrivateNodeInterface::GraphPtr(dep1);

    return Event<std::tuple<U1, Us ...>, unique>( CtorTag{ }, std::make_shared<EventJoinNode<U1, Us ...>>(
        graphPtr, PrivateEventLinkNodeInterface::GetLocalNodePtr(graphPtr, dep1), PrivateEventLinkNodeInterface::GetLocalNodePtr(graphPtr, deps) ...));
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
bool Equals(const EventBase<L>& lhs, const EventBase<R>& rhs)
{
    return lhs.Equals(rhs);
}

struct PrivateEventLinkNodeInterface
{
    template <typename E>
    static auto GetLocalNodePtr(const std::shared_ptr<ReactiveGraph>& targetGraph, const EventBase<E>& dep) -> std::shared_ptr<EventStreamNode<E>>
    {
        const std::shared_ptr<ReactiveGraph>& sourceGraph = PrivateNodeInterface::GraphPtr(dep);

        if (sourceGraph == targetGraph)
        {
            return PrivateNodeInterface::NodePtr(dep);
        }
        else
        {
            return EventLinkBase<E>::CreateLinkNode(targetGraph, dep);
        }
    }
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_EVENT_H_INCLUDED