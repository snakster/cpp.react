
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

template <typename T = Token>
class EventBase
{
private:
    using NodeType = REACT_IMPL::EventStreamNode<T>;

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

    template <typename FIn, typename TIn>
    auto CreateProcessingNode(FIn&& func, const EventBase<TIn>& dep) -> decltype(auto)
    {
        using F = typename std::decay<FIn>::type;
        using ProcessingNodeType = REACT_IMPL::EventProcessingNode<T, TIn, F>;

        return std::make_shared<ProcessingNodeType>(
            REACT_IMPL::PrivateNodeInterface::GraphPtr(dep),
            REACT_IMPL::PrivateNodeInterface::NodePtr(dep),
            std::forward<F>(func));
    }

private:
    std::shared_ptr<NodeType> nodePtr_;

    friend struct REACT_IMPL::PrivateNodeInterface;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventSource
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T = Token>
class EventSourceBase : public EventBase<T>
{
private:
    using NodeType = REACT_IMPL::EventSourceNode<T>;

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
    
    void Emit(const T& value)
        { EmitValue(value); }

    void Emit(T&& value)
        { EmitValue(std::move(value)); }

    template <typename U = T, typename = typename std::enable_if<std::is_same<U,Token>::value>::type>
    void Emit()
        { EmitValue(Token::value); }

    EventSourceBase& operator<<(const T& value)
        { EmitValue(e); return *this; }

    EventSourceBase& operator<<(T&& value)
        { EmitValue(std::move(value)); return *this; }

private:
    template <typename U>
    void EmitValue(U&& value)
    {
        using REACT_IMPL::NodeId;
        using REACT_IMPL::IReactiveGraph;

        NodeType* castedPtr = static_cast<NodeType*>(this->NodePtr().get());

        NodeId nodeId = castedPtr->GetNodeId();
        auto& graphPtr = NodePtr()->GraphPtr();
        graphPtr->AddInput(nodeId, [castedPtr, &value] { castedPtr->EmitValue(std::forward<U>(value)); });
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Event
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
class Event<T, unique> : public EventBase<T>
{
public:
    using EventBase::EventBase;

    using ValueType = T;

    Event() = delete;

    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;

    Event(Event&&) = default;
    Event& operator=(Event&&) = default;

    template <typename F, typename U>
    Event(F&& func, const EventBase<U>& dep) :
        Event::EventBase( CreateProcessingNode(std::forward<F>(func), dep) )
        { }
};

template <typename T>
class Event<T, shared> : public EventBase<T>
{
public:
    using EventBase::EventBase;

    using ValueType = T;

    Event() = delete;

    Event(const Event&) = default;
    Event& operator=(const Event&) = default;

    Event(Event&&) = default;
    Event& operator=(Event&&) = default;

    Event(Event<T, unique>&& other) :
        Event::EventBase( std::move(other) )
        { }

    Event& operator=(Event<T, unique>&& other)
        { Event::EventBase::operator=(std::move(other)); return *this; }

    template <typename F, typename U>
    Event(F&& func, const EventBase<U>& dep) :
        Event::EventBase( CreateProcessingNode(std::forward<F>(func), dep) )
        { }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventSource
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
class EventSource<T, unique> : public EventSourceBase<T>
{
public:
    using EventSourceBase::EventSourceBase;

    using ValueType = T;

    EventSource() = delete;

    EventSource(const EventSource&) = delete;
    EventSource& operator=(const EventSource&) = delete;

    EventSource(EventSource&&) = default;
    EventSource& operator=(EventSource&&) = default;
};

template <typename T>
class EventSource<T, shared> : public EventSourceBase<T>
{
public:
    using EventSourceBase::EventSourceBase;

    using ValueType = T;

    EventSource() = delete;

    EventSource(const EventSource&) = default;
    EventSource& operator=(const EventSource&) = default;

    EventSource(EventSource&&) = default;
    EventSource& operator=(EventSource&&) = default;

    EventSource(EventSource<T, unique>&& other) :
        EventSource::EventSourceBase( std::move(other) )
        { }

    EventSource& operator=(EventSource<T, unique>&& other)
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

    static_assert(sizeof...(Us) > 0, "Merge: 2+ arguments are required.");

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
template <typename F, typename T>
auto Filter(F&& pred, const EventBase<T>& dep) -> Event<T, unique>
{
    auto filterFunc = [capturedPred = std::forward<F>(pred)] (EventRange<T> inRange, EventSink<T> out)
        { std::copy_if(inRange.begin(), inRange.end(), out, capturedPred); };

    return Event<T, unique>(std::move(filterFunc), dep);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Transform
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T, typename F, typename U>
auto Transform(F&& op, const EventBase<U>& dep) -> Event<T, unique>
{
    auto transformFunc = [capturedPred = std::forward<F>(op)] (EventRange<U> inRange, EventSink<T> out)
        { std::transform(inRange.begin(), inRange.end(), out, pred); };

    return Event<T, unique>(std::move(transformFunc), dep);
}

#if 0

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Filter - Synced
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename E,
    typename FIn,
    typename ... TDepValues
>
auto Filter(const Events<E>& source, const SignalPack<TDepValues...>& depPack, FIn&& func) -> Events<E>
{
    using REACT_IMPL::SyncedEventFilterNode;

    using F = typename std::decay<FIn>::type;

    struct NodeBuilder_
    {
        NodeBuilder_(const Events<E>& source, FIn&& func) :
            MySource( source ),
            MyFunc( std::forward<FIn>(func) )
        {}

        auto operator()(const Signal<D,TDepValues>& ... deps)
            -> Events<E>
        {
            return Events<E>(
                std::make_shared<SyncedEventFilterNode<D,E,F,TDepValues ...>>(
                     GetNodePtr(MySource), std::forward<FIn>(MyFunc), GetNodePtr(deps) ...));
        }

        const Events<E>&      MySource;
        FIn                     MyFunc;
    };

    return REACT_IMPL::apply(
        NodeBuilder_( source, std::forward<FIn>(func) ),
        depPack.Data);
}



///////////////////////////////////////////////////////////////////////////////////////////////////
/// Transform - Synced
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename TIn,
    typename FIn,
    typename ... TDepValues,
    typename TOut = typename std::result_of<FIn(TIn,TDepValues...)>::type
>
auto Transform(const Events<TIn>& source, FIn&& func, const Signal<TDepValues>& ... deps) -> Events<TOut>
{
    using REACT_IMPL::SyncedEventTransformNode;

    using F = typename std::decay<FIn>::type;

    struct NodeBuilder_
    {
        NodeBuilder_(const Events<TIn>& source, FIn&& func) :
            MySource( source ),
            MyFunc( std::forward<FIn>(func) )
        {}

        auto operator()(const Signal<D,TDepValues>& ... deps)
            -> Events<TOut>
        {
            return Events<TOut>(
                std::make_shared<SyncedEventTransformNode<D,TIn,TOut,F,TDepValues ...>>(
                     GetNodePtr(MySource), std::forward<FIn>(MyFunc), GetNodePtr(deps) ...));
        }

        const Events<TIn>&    MySource;
        FIn                     MyFunc;
    };

    return REACT_IMPL::apply(
        NodeBuilder_( source, std::forward<FIn>(func) ),
        depPack.Data);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Process - Synced
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename TOut,
    typename TIn,
    typename FIn,
    typename ... TDepValues
>
auto Process(const Events<TIn>& source, const SignalPack<D,TDepValues...>& depPack, FIn&& func) -> Events<TOut>
{
    using REACT_IMPL::SyncedEventProcessingNode;

    using F = typename std::decay<FIn>::type;

    struct NodeBuilder_
    {
        NodeBuilder_(const Events<TIn>& source, FIn&& func) :
            MySource( source ),
            MyFunc( std::forward<FIn>(func) )
        {}

        auto operator()(const Signal<D,TDepValues>& ... deps)
            -> Events<TOut>
        {
            return Events<TOut>(
                std::make_shared<SyncedEventProcessingNode<D,TIn,TOut,F,TDepValues ...>>(
                     GetNodePtr(MySource), std::forward<FIn>(MyFunc), GetNodePtr(deps) ...));
        }

        const Events<TIn>&    MySource;
        FIn                     MyFunc;
    };

    return REACT_IMPL::apply(
        NodeBuilder_( source, std::forward<FIn>(func) ),
        depPack.Data);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Flatten
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TInnerValue>
auto Flatten(const Signal<Events<TInnerValue>>& outer) -> Events<TInnerValue>
{
    return Events<TInnerValue>(
        std::make_shared<REACT_IMPL::EventFlattenNode<Events<TInnerValue>, TInnerValue>>(
            GetNodePtr(outer), GetNodePtr(outer.Value())));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Join
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename ... TArgs>
auto Join(const Events<TArgs>& ... args) -> Events<std::tuple<TArgs ...>>
{
    using REACT_IMPL::EventJoinNode;

    static_assert(sizeof...(TArgs) > 1, "Join: 2+ arguments are required.");

    return Events< std::tuple<TArgs ...>>(
        std::make_shared<EventJoinNode<TArgs...>>(
            GetNodePtr(args) ...));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Token
///////////////////////////////////////////////////////////////////////////////////////////////////
enum class Token { value };

struct Tokenizer
{
    template <typename T>
    Token operator()(const T&) const { return Token::value; }
};

template <typename T>
auto Tokenize(T&& source) -> decltype(auto)
{
    return Transform(source, Tokenizer{ });
}

#endif

/******************************************/ REACT_END /******************************************/

/***************************************/ REACT_IMPL_BEGIN /**************************************/

template <typename L, typename R>
bool Equals(const EventBase<L>& lhs, const EventBase<R>& rhs)
{
    return lhs.Equals(rhs);
}

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_EVENT_H_INCLUDED