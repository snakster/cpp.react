
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_EVENT_H_INCLUDED
#define REACT_EVENT_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <memory>
#include <type_traits>
#include <utility>

#include "react/Observer.h"
#include "react/TypeTraits.h"
#include "react/common/Util.h"
#include "react/detail/EventBase.h"

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward declarations
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename E>
class Events;

template <typename D, typename E>
class EventSource;

template <typename D, typename E, typename TOp>
class TempEvents;

enum class Token;

template <typename D, typename S>
class Signal;

template <typename D, typename ... TValues>
class SignalPack;

using REACT_IMPL::WeightHint;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MakeEventSource
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename E = Token>
auto MakeEventSource()
    -> EventSource<D,E>
{
    using REACT_IMPL::EventSourceNode;

    return EventSource<D,E>(
        std::make_shared<EventSourceNode<D,E>>());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Merge
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename TArg1,
    typename ... TArgs,
    typename E = TArg1,
    typename TOp = REACT_IMPL::EventMergeOp<E,
        REACT_IMPL::EventStreamNodePtrT<D,TArg1>,
        REACT_IMPL::EventStreamNodePtrT<D,TArgs> ...>
>
auto Merge(const Events<D,TArg1>& arg1, const Events<D,TArgs>& ... args)
    -> TempEvents<D,E,TOp>
{
    using REACT_IMPL::EventOpNode;

    static_assert(sizeof...(TArgs) > 0,
        "Merge: 2+ arguments are required.");

    return TempEvents<D,E,TOp>(
        std::make_shared<EventOpNode<D,E,TOp>>(
            GetNodePtr(arg1), GetNodePtr(args) ...));
}

template
<
    typename TLeftEvents,
    typename TRightEvents,
    typename D = typename TLeftEvents::DomainT,
    typename TLeftVal = typename TLeftEvents::ValueT,
    typename TRightVal = typename TRightEvents::ValueT,
    typename E = TLeftVal,
    typename TOp = REACT_IMPL::EventMergeOp<E,
        REACT_IMPL::EventStreamNodePtrT<D,TLeftVal>,
        REACT_IMPL::EventStreamNodePtrT<D,TRightVal>>,
    class = typename std::enable_if<
        IsEvent<TLeftEvents>::value>::type,
    class = typename std::enable_if<
        IsEvent<TRightEvents>::value>::type
>
auto operator|(const TLeftEvents& lhs, const TRightEvents& rhs)
    -> TempEvents<D,E,TOp>
{
    using REACT_IMPL::EventOpNode;

    return TempEvents<D,E,TOp>(
        std::make_shared<EventOpNode<D,E,TOp>>(
            GetNodePtr(lhs), GetNodePtr(rhs)));
}

template
<
    typename D,
    typename TLeftVal,
    typename TLeftOp,
    typename TRightVal,
    typename TRightOp,
    typename E = TLeftVal,
    typename TOp = REACT_IMPL::EventMergeOp<E,TLeftOp,TRightOp>
>
auto operator|(TempEvents<D,TLeftVal,TLeftOp>&& lhs, TempEvents<D,TRightVal,TRightOp>&& rhs)
    -> TempEvents<D,E,TOp>
{
    using REACT_IMPL::EventOpNode;

    return TempEvents<D,E,TOp>(
        std::make_shared<EventOpNode<D,E,TOp>>(
            lhs.StealOp(), rhs.StealOp()));
}

template
<
    typename D,
    typename TLeftVal,
    typename TLeftOp,
    typename TRightEvents,
    typename TRightVal = typename TRightEvents::ValueT,
    typename E = TLeftVal,
    typename TOp = REACT_IMPL::EventMergeOp<E,
        TLeftOp,
        REACT_IMPL::EventStreamNodePtrT<D,TRightVal>>,
    class = typename std::enable_if<
        IsEvent<TRightEvents>::value>::type
>
auto operator|(TempEvents<D,TLeftVal,TLeftOp>&& lhs, const TRightEvents& rhs)
    -> TempEvents<D,E,TOp>
{
    using REACT_IMPL::EventOpNode;

    return TempEvents<D,E,TOp>(
        std::make_shared<EventOpNode<D,E,TOp>>(
            lhs.StealOp(), GetNodePtr(rhs)));
}

template
<
    typename TLeftEvents,
    typename D,
    typename TRightVal,
    typename TRightOp,
    typename TLeftVal = typename TLeftEvents::ValueT,
    typename E = TLeftVal,
    typename TOp = REACT_IMPL::EventMergeOp<E,
        REACT_IMPL::EventStreamNodePtrT<D,TRightVal>,
        TRightOp>,
    class = typename std::enable_if<
        IsEvent<TLeftEvents>::value>::type
>
auto operator|(const TLeftEvents& lhs, TempEvents<D,TRightVal,TRightOp>&& rhs)
    -> TempEvents<D,E,TOp>
{
    using REACT_IMPL::EventOpNode;

    return TempEvents<D,E,TOp>(
        std::make_shared<EventOpNode<D,E,TOp>>(
            GetNodePtr(lhs), rhs.StealOp()));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Filter
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename E,
    typename FIn,
    typename F = typename std::decay<FIn>::type,
    typename TOp = REACT_IMPL::EventFilterOp<E,F,
        REACT_IMPL::EventStreamNodePtrT<D,E>>
>
auto Filter(const Events<D,E>& src, FIn&& filter)
    -> TempEvents<D,E,TOp>
{
    using REACT_IMPL::EventOpNode;

    return TempEvents<D,E,TOp>(
        std::make_shared<EventOpNode<D,E,TOp>>(
            std::forward<FIn>(filter), GetNodePtr(src)));
}

template
<
    typename D,
    typename E,
    typename TOpIn,
    typename FIn,
    typename F = typename std::decay<FIn>::type,
    typename TOpOut = REACT_IMPL::EventFilterOp<E,F,TOpIn>
>
auto Filter(TempEvents<D,E,TOpIn>&& src, FIn&& filter)
    -> TempEvents<D,E,TOpOut>
{
    using REACT_IMPL::EventOpNode;

    return TempEvents<D,E,TOpOut>(
        std::make_shared<EventOpNode<D,E,TOpOut>>(
            std::forward<FIn>(filter), src.StealOp()));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Filter - Synced
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename E,
    typename FIn,
    typename ... TDepValues
>
auto Filter(const Events<D,E>& source, const SignalPack<D,TDepValues...>& depPack, FIn&& func)
    -> Events<D,E>
{
    using REACT_IMPL::SyncedEventFilterNode;

    using F = typename std::decay<FIn>::type;

    struct NodeBuilder_
    {
        NodeBuilder_(const Events<D,E>& source, FIn&& func) :
            MySource( source ),
            MyFunc( std::forward<FIn>(func) )
        {}

        auto operator()(const Signal<D,TDepValues>& ... deps)
            -> Events<D,E>
        {
            return Events<D,E>(
                std::make_shared<SyncedEventFilterNode<D,E,F,TDepValues ...>>(
                     GetNodePtr(MySource), std::forward<FIn>(MyFunc), GetNodePtr(deps) ...));
        }

        const Events<D,E>&      MySource;
        FIn                     MyFunc;
    };

    return REACT_IMPL::apply(
        NodeBuilder_( source, std::forward<FIn>(func) ),
        depPack.Data);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Transform
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename TIn,
    typename FIn,
    typename F = typename std::decay<FIn>::type,
    typename TOut = typename std::result_of<F(TIn)>::type,
    typename TOp = REACT_IMPL::EventTransformOp<TIn,F,
        REACT_IMPL::EventStreamNodePtrT<D,TIn>>
>
auto Transform(const Events<D,TIn>& src, FIn&& func)
    -> TempEvents<D,TOut,TOp>
{
    using REACT_IMPL::EventOpNode;

    return TempEvents<D,TOut,TOp>(
        std::make_shared<EventOpNode<D,TOut,TOp>>(
            std::forward<FIn>(func), GetNodePtr(src)));
}

template
<
    typename D,
    typename TIn,
    typename TOpIn,
    typename FIn,
    typename F = typename std::decay<FIn>::type,
    typename TOut = typename std::result_of<F(TIn)>::type,
    typename TOpOut = REACT_IMPL::EventTransformOp<TIn,F,TOpIn>
>
auto Transform(TempEvents<D,TIn,TOpIn>&& src, FIn&& func)
    -> TempEvents<D,TOut,TOpOut>
{
    using REACT_IMPL::EventOpNode;

    return TempEvents<D,TOut,TOpOut>(
        std::make_shared<EventOpNode<D,TOut,TOpOut>>(
            std::forward<FIn>(func), src.StealOp()));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Transform - Synced
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename TIn,
    typename FIn,
    typename ... TDepValues,
    typename TOut = typename std::result_of<FIn(TIn,TDepValues...)>::type
>
auto Transform(const Events<D,TIn>& source, const SignalPack<D,TDepValues...>& depPack, FIn&& func)
    -> Events<D,TOut>
{
    using REACT_IMPL::SyncedEventTransformNode;

    using F = typename std::decay<FIn>::type;

    struct NodeBuilder_
    {
        NodeBuilder_(const Events<D,TIn>& source, FIn&& func) :
            MySource( source ),
            MyFunc( std::forward<FIn>(func) )
        {}

        auto operator()(const Signal<D,TDepValues>& ... deps)
            -> Events<D,TOut>
        {
            return Events<D,TOut>(
                std::make_shared<SyncedEventTransformNode<D,TIn,TOut,F,TDepValues ...>>(
                     GetNodePtr(MySource), std::forward<FIn>(MyFunc), GetNodePtr(deps) ...));
        }

        const Events<D,TIn>&    MySource;
        FIn                     MyFunc;
    };

    return REACT_IMPL::apply(
        NodeBuilder_( source, std::forward<FIn>(func) ),
        depPack.Data);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Process
///////////////////////////////////////////////////////////////////////////////////////////////////
using REACT_IMPL::EventRange;
using REACT_IMPL::EventEmitter;

template
<
    typename TOut,
    typename D,
    typename TIn,
    typename FIn,
    typename F = typename std::decay<FIn>::type
>
auto Process(const Events<D,TIn>& src, FIn&& func)
    -> Events<D,TOut>
{
    using REACT_IMPL::EventProcessingNode;

    return Events<D,TOut>(
        std::make_shared<EventProcessingNode<D,TIn,TOut,F>>(
            GetNodePtr(src), std::forward<FIn>(func)));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Process - Synced
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename TOut,
    typename D,
    typename TIn,
    typename FIn,
    typename ... TDepValues
>
auto Process(const Events<D,TIn>& source, const SignalPack<D,TDepValues...>& depPack, FIn&& func)
    -> Events<D,TOut>
{
    using REACT_IMPL::SyncedEventProcessingNode;

    using F = typename std::decay<FIn>::type;

    struct NodeBuilder_
    {
        NodeBuilder_(const Events<D,TIn>& source, FIn&& func) :
            MySource( source ),
            MyFunc( std::forward<FIn>(func) )
        {}

        auto operator()(const Signal<D,TDepValues>& ... deps)
            -> Events<D,TOut>
        {
            return Events<D,TOut>(
                std::make_shared<SyncedEventProcessingNode<D,TIn,TOut,F,TDepValues ...>>(
                     GetNodePtr(MySource), std::forward<FIn>(MyFunc), GetNodePtr(deps) ...));
        }

        const Events<D,TIn>&    MySource;
        FIn                     MyFunc;
    };

    return REACT_IMPL::apply(
        NodeBuilder_( source, std::forward<FIn>(func) ),
        depPack.Data);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Flatten
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename TInnerValue
>
auto Flatten(const Signal<D,Events<D,TInnerValue>>& outer)
    -> Events<D,TInnerValue>
{
    return Events<D,TInnerValue>(
        std::make_shared<REACT_IMPL::EventFlattenNode<D, Events<D,TInnerValue>, TInnerValue>>(
            GetNodePtr(outer), GetNodePtr(outer.Value())));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Join
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename ... TArgs
>
auto Join(const Events<D,TArgs>& ... args)
    -> Events<D, std::tuple<TArgs ...>>
{
    using REACT_IMPL::EventJoinNode;

    static_assert(sizeof...(TArgs) > 1,
        "Join: 2+ arguments are required.");

    return Events<D, std::tuple<TArgs ...>>(
        std::make_shared<EventJoinNode<D,TArgs...>>(
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

template <typename TEvents>
auto Tokenize(TEvents&& source)
    -> decltype(Transform(source, Tokenizer{}))
{
    return Transform(source, Tokenizer{});
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Events
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename E = Token
>
class Events : public REACT_IMPL::EventStreamBase<D,E>
{
private:
    using NodeT     = REACT_IMPL::EventStreamNode<D,E>;
    using NodePtrT  = std::shared_ptr<NodeT>;

public:
    using ValueT = E;

    // Default ctor
    Events() = default;

    // Copy ctor
    Events(const Events&) = default;

    // Move ctor
    Events(Events&& other) :
        Events::EventStreamBase( std::move(other) )
    {}

    // Node ctor
    explicit Events(NodePtrT&& nodePtr) :
        Events::EventStreamBase( std::move(nodePtr) )
    {}

    // Copy assignment
    Events& operator=(const Events&) = default;

    // Move assignment
    Events& operator=(Events&& other)
    {
        Events::EventStreamBase::operator=( std::move(other) );
        return *this;
    }

    bool Equals(const Events& other) const
    {
        return Events::EventStreamBase::Equals(other);
    }

    bool IsValid() const
    {
        return Events::EventStreamBase::IsValid();
    }

    void SetWeightHint(WeightHint weight)
    {
        Events::EventStreamBase::SetWeightHint(weight);
    }

    auto Tokenize() const
        -> decltype(REACT::Tokenize(std::declval<Events>()))
    {
        return REACT::Tokenize(*this);
    }

    template <typename ... TArgs>
    auto Merge(TArgs&& ... args) const
        -> decltype(REACT::Merge(std::declval<Events>(), std::forward<TArgs>(args) ...))
    {
        return REACT::Merge(*this, std::forward<TArgs>(args) ...);
    }

    template <typename F>
    auto Filter(F&& f) const
        -> decltype(REACT::Filter(std::declval<Events>(), std::forward<F>(f)))
    {
        return REACT::Filter(*this, std::forward<F>(f));
    }

    template <typename F>
    auto Transform(F&& f) const
        -> decltype(REACT::Transform(std::declval<Events>(), std::forward<F>(f)))
    {
        return REACT::Transform(*this, std::forward<F>(f));
    }
};

// Specialize for references
template
<
    typename D,
    typename E
>
class Events<D,E&> : public REACT_IMPL::EventStreamBase<D,std::reference_wrapper<E>>
{
private:
    using NodeT     = REACT_IMPL::EventStreamNode<D,std::reference_wrapper<E>>;
    using NodePtrT  = std::shared_ptr<NodeT>;

public:
    using ValueT = E;

    // Default ctor
    Events() = default;

    // Copy ctor
    Events(const Events&) = default;

    // Move ctor
    Events(Events&& other) :
        Events::EventStreamBase( std::move(other) )
    {}

    // Node ctor
    explicit Events(NodePtrT&& nodePtr) :
        Events::EventStreamBase( std::move(nodePtr) )
    {}

    // Copy assignment
    Events& operator=(const Events&) = default;

    // Move assignment
    Events& operator=(Events&& other)
    {
        Events::EventStreamBase::operator=( std::move(other) );
        return *this;
    }

    bool Equals(const Events& other) const
    {
        return Events::EventStreamBase::Equals(other);
    }

    bool IsValid() const
    {
        return Events::EventStreamBase::IsValid();
    }

    void SetWeightHint(WeightHint weight)
    {
        Events::EventStreamBase::SetWeightHint(weight);
    }

    auto Tokenize() const
        -> decltype(REACT::Tokenize(std::declval<Events>()))
    {
        return REACT::Tokenize(*this);
    }

    template <typename ... TArgs>
    auto Merge(TArgs&& ... args)
        -> decltype(REACT::Merge(std::declval<Events>(), std::forward<TArgs>(args) ...))
    {
        return REACT::Merge(*this, std::forward<TArgs>(args) ...);
    }

    template <typename F>
    auto Filter(F&& f) const
        -> decltype(REACT::Filter(std::declval<Events>(), std::forward<F>(f)))
    {
        return REACT::Filter(*this, std::forward<F>(f));
    }

    template <typename F>
    auto Transform(F&& f) const
        -> decltype(REACT::Transform(std::declval<Events>(), std::forward<F>(f)))
    {
        return REACT::Transform(*this, std::forward<F>(f));
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventSource
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename E = Token
>
class EventSource : public Events<D,E>
{
private:
    using NodeT     = REACT_IMPL::EventSourceNode<D,E>;
    using NodePtrT  = std::shared_ptr<NodeT>;

public:
    // Default ctor
    EventSource() = default;

    // Copy ctor
    EventSource(const EventSource&) = default;

    // Move ctor
    EventSource(EventSource&& other) :
        EventSource::Events( std::move(other) )
    {}

    // Node ctor
    explicit EventSource(NodePtrT&& nodePtr) :
        EventSource::Events( std::move(nodePtr) )
    {}

    // Copy assignemnt
    EventSource& operator=(const EventSource&) = default;

    // Move assignment
    EventSource& operator=(EventSource&& other)
    {
        EventSource::Events::operator=( std::move(other) );
        return *this;
    }

    // Explicit emit
    void Emit(const E& e) const     { EventSource::EventStreamBase::emit(e); }
    void Emit(E&& e) const          { EventSource::EventStreamBase::emit(std::move(e)); }

    void Emit() const
    {
        static_assert(std::is_same<E,Token>::value, "Can't emit on non token stream.");
        EventSource::EventStreamBase::emit(Token::value);
    }

    // Function object style
    void operator()(const E& e) const   { EventSource::EventStreamBase::emit(e); }
    void operator()(E&& e) const        { EventSource::EventStreamBase::emit(std::move(e)); }

    void operator()() const
    {
        static_assert(std::is_same<E,Token>::value, "Can't emit on non token stream.");
        EventSource::EventStreamBase::emit(Token::value);
    }

    // Stream style
    const EventSource& operator<<(const E& e) const
    {
        EventSource::EventStreamBase::emit(e);
        return *this;
    }

    const EventSource& operator<<(E&& e) const
    {
        EventSource::EventStreamBase::emit(std::move(e));
        return *this;
    }
};

// Specialize for references
template
<
    typename D,
    typename E
>
class EventSource<D,E&> : public Events<D,std::reference_wrapper<E>>
{
private:
    using NodeT     = REACT_IMPL::EventSourceNode<D,std::reference_wrapper<E>>;
    using NodePtrT  = std::shared_ptr<NodeT>;

public:
    // Default ctor
    EventSource() = default;

    // Copy ctor
    EventSource(const EventSource&) = default;

    // Move ctor
    EventSource(EventSource&& other) :
        EventSource::Events( std::move(other) )
    {}

    // Node ctor
    explicit EventSource(NodePtrT&& nodePtr) :
        EventSource::Events( std::move(nodePtr) )
    {}

    // Copy assignment
    EventSource& operator=(const EventSource&) = default;

    // Move assignment
    EventSource& operator=(EventSource&& other)
    {
        EventSource::Events::operator=( std::move(other) );
        return *this;
    }

    // Explicit emit
    void Emit(std::reference_wrapper<E> e) const        { EventSource::EventStreamBase::emit(e); }

    // Function object style
    void operator()(std::reference_wrapper<E> e) const  { EventSource::EventStreamBase::emit(e); }

    // Stream style
    const EventSource& operator<<(std::reference_wrapper<E> e) const
    {
        EventSource::EventStreamBase::emit(e);
        return *this;
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// TempEvents
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename E,
    typename TOp
>
class TempEvents : public Events<D,E>
{
protected:
    using NodeT     = REACT_IMPL::EventOpNode<D,E,TOp>;
    using NodePtrT  = std::shared_ptr<NodeT>;

public:
    // Default ctor
    TempEvents() = default;

    // Copy ctor
    TempEvents(const TempEvents&) = default;

    // Move ctor
    TempEvents(TempEvents&& other) :
        TempEvents::Events( std::move(other) )
    {}

    // Node ctor
    explicit TempEvents(NodePtrT&& nodePtr) :
        TempEvents::Events( std::move(nodePtr) )
    {}

    // Copy assignment
    TempEvents& operator=(const TempEvents&) = default;

    // Move assignment
    TempEvents& operator=(TempEvents&& other)
    {
        TempEvents::EventStreamBase::operator=( std::move(other) );
        return *this;
    }

    TOp StealOp()
    {
        return std::move(reinterpret_cast<NodeT*>(this->ptr_.get())->StealOp());
    }

    template <typename ... TArgs>
    auto Merge(TArgs&& ... args)
        -> decltype(REACT::Merge(std::declval<TempEvents>(), std::forward<TArgs>(args) ...))
    {
        return REACT::Merge(*this, std::forward<TArgs>(args) ...);
    }

    template <typename F>
    auto Filter(F&& f) const
        -> decltype(REACT::Filter(std::declval<TempEvents>(), std::forward<F>(f)))
    {
        return REACT::Filter(*this, std::forward<F>(f));
    }

    template <typename F>
    auto Transform(F&& f) const
        -> decltype(REACT::Transform(std::declval<TempEvents>(), std::forward<F>(f)))
    {
        return REACT::Transform(*this, std::forward<F>(f));
    }
};

/******************************************/ REACT_END /******************************************/

/***************************************/ REACT_IMPL_BEGIN /**************************************/

template <typename D, typename L, typename R>
bool Equals(const Events<D,L>& lhs, const Events<D,R>& rhs)
{
    return lhs.Equals(rhs);
}

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_EVENT_H_INCLUDED