
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
    EventBase() = default;

    EventBase(const EventBase&) = default;
    EventBase& operator=(const EventBase&) = default;

    EventBase(EventBase&&) = default;
    EventBase& operator=(EventBase&&) = default;

    ~EventBase() = default;

    // Node ctor
    explicit EventBase(std::shared_ptr<NodeType>&& nodePtr) :
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
    auto NodePtr() -> std::shared_ptr<NodeType>&
        { return nodePtr_; }

    auto NodePtr() const -> const std::shared_ptr<NodeType>&
        { return nodePtr_; }

    template <typename F, typename T>
    auto CreateProcessingNode(F&& func, const EventBase<T>& dep) -> decltype(auto)
    {
        using ProcessingNodeType = REACT_IMPL::EventProcessingNode<E, T, typename std::decay<F>::type>;

        return std::make_shared<ProcessingNodeType>(
            REACT_IMPL::PrivateNodeInterface::GraphPtr(dep),
            std::forward<F>(func),
            REACT_IMPL::PrivateNodeInterface::NodePtr(dep));
    }

    template <typename F, typename T, typename ... Us>
    auto CreateSyncedProcessingNode(F&& func, const EventBase<T>& dep, const SignalBase<Us> ... syncs) -> decltype(auto)
    {
        using SyncedProcessingNodeType = REACT_IMPL::SyncedEventProcessingNode<E, T, typename std::decay<F>::type, Us ...>;

        return std::make_shared<SyncedProcessingNodeType>(
            REACT_IMPL::GetCheckedGraphPtr(dep, syncs ...),
            std::forward<F>(func),
            REACT_IMPL::PrivateNodeInterface::NodePtr(dep), REACT_IMPL::PrivateNodeInterface::NodePtr(syncs) ...);
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
private:
    using NodeType = REACT_IMPL::EventSourceNode<E>;

public:
    using EventBase::EventBase;

    EventSourceBase() = default;

    EventSourceBase(const EventSourceBase&) = default;
    EventSourceBase& operator=(const EventSourceBase&) = default;

    EventSourceBase(EventSourceBase&& other) = default;
    EventSourceBase& operator=(EventSourceBase&& other) = default;

    template <typename TGroup>
    explicit EventSourceBase(const TGroup& group) :
        EventSourceBase::EventBase( std::make_shared<NodeType>(REACT_IMPL::PrivateReactiveGroupInterface::GraphPtr(group)) )
        { }
    
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

private:
    template <typename T>
    void EmitValue(T&& value)
    {
        using REACT_IMPL::NodeId;
        using REACT_IMPL::IReactiveGraph;

        NodeType* castedPtr = static_cast<NodeType*>(this->NodePtr().get());

        NodeId nodeId = castedPtr->GetNodeId();
        auto& graphPtr = NodePtr()->GraphPtr();
        graphPtr->AddInput(nodeId, [castedPtr, &value] { castedPtr->EmitValue(std::forward<T>(value)); });
    }
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
        Event::EventBase( CreateProcessingNode(std::forward<F>(func), dep) )
        { }

    template <typename F, typename T, typename ... Us>
    Event(F&& func, const EventBase<T>& dep, const SignalBase<Us> ... signals) :
        Event::EventBase( CreateSyncedProcessingNode(std::forward<F>(func), dep, signals ...) )
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
        Event::EventBase( CreateProcessingNode(std::forward<F>(func), dep) )
        { }

    template <typename F, typename T, typename ... Us>
    Event(F&& func, const EventBase<T>& dep, const SignalBase<Us> ... signals) :
        Event::EventBase( SyncedCreateProcessingNode(std::forward<F>(func), dep, signals ...) )
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

    EventSource(EventSource<E, unique>&& other) :
        EventSource::EventSourceBase( std::move(other) )
        { }

    EventSource& operator=(EventSource<E, unique>&& other)
        { EventSource::EventSourceBase::operator=(std::move(other)); return *this; }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Merge
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T = void, typename U1, typename ... Us>
auto Merge(const EventBase<U1>& dep1, const EventBase<Us>& ... deps) -> decltype(auto)
{
    using REACT_IMPL::EventMergeNode;
    using REACT_IMPL::GetCheckedGraphPtr;
    using REACT_IMPL::PrivateNodeInterface;

    static_assert(sizeof...(Us) > 0, "Merge requires at least 2 inputs.");

    // If supplied, use merge type, otherwise default to common type.
    using E = typename std::conditional<
        std::is_same<T, void>::value,
        typename std::common_type<U1, Us ...>::type,
        T>::type;

    const auto& graphPtr = GetCheckedGraphPtr(dep1, deps ...);

    return Event<E, unique>(
        std::make_shared<EventMergeNode<E, U1, Us ...>>(graphPtr, PrivateNodeInterface::NodePtr(dep1), PrivateNodeInterface::NodePtr(deps) ...));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Filter
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename F, typename E>
auto Filter(F&& pred, const EventBase<E>& dep) -> Event<E, unique>
{
    auto filterFunc = [capturedPred = std::forward<F>(pred)] (EventRange<E> inRange, EventSink<E> out)
        { std::copy_if(inRange.begin(), inRange.end(), out, capturedPred); };

    return Event<E, unique>(std::move(filterFunc), dep);
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
auto Transform(F&& op, const EventBase<T>& dep) -> Event<E, unique>
{
    auto transformFunc = [capturedOp = std::forward<F>(op)] (EventRange<T> inRange, EventSink<E> out)
        { std::transform(inRange.begin(), inRange.end(), out, capturedOp); };

    return Event<E, unique>(std::move(transformFunc), dep);
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
template <typename ... Ts>
auto Join(const EventBase<Ts>& ... deps) -> Event<std::tuple<Ts ...>, unique>
{
    using REACT_IMPL::EventJoinNode;
    using REACT_IMPL::GetCheckedGraphPtr;
    using REACT_IMPL::PrivateNodeInterface;

    static_assert(sizeof...(Ts) > 1, "Join requires at least 2 inputs.");

    // If supplied, use merge type, otherwise default to common type.
    const auto& graphPtr = GetCheckedGraphPtr(deps ...);

    return Event<std::tuple<Ts ...>, unique>(
        std::make_shared<EventJoinNode<Ts ...>>(graphPtr, PrivateNodeInterface::NodePtr(deps) ...));
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

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_EVENT_H_INCLUDED