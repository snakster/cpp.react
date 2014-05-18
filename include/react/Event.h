
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/detail/Defs.h"

#include <memory>
#include <type_traits>
#include <utility>

#include "react/Observer.h"
#include "react/TypeTraits.h"
#include "react/detail/EventBase.h"

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventToken
///////////////////////////////////////////////////////////////////////////////////////////////////
enum class EventToken { token };

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Events
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename E = EventToken
>
class Events : public REACT_IMPL::EventStreamBase<D,E>
{
protected:
    using BaseT     = REACT_IMPL::EventStreamBase<D,E>;

private:
    using NodeT     = REACT_IMPL::EventStreamNode<D,E>;
    using NodePtrT  = REACT_IMPL::SharedPtrT<NodeT>;

public:
    using ValueT = E;

    Events() = default;
    Events(const Events&) = default;

    Events(Events&& other) :
        EventStreamBase{ std::move(other) }
    {}

    explicit Events(NodePtrT&& nodePtr) :
        EventStreamBase{ std::move(nodePtr) }
    {}

    bool Equals(const Events& other) const
    {
        return BaseT::Equals(other);
    }

    bool IsValid() const
    {
        return BaseT::IsValid();
    }

    auto Forward() const
        -> decltype(REACT::Forward(std::declval<Events>()))
    {
        return REACT::Forward(*this);
    }

    template <typename ... TArgs>
    auto Merge(const Events<D,TArgs>& ... args) const
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

    template <typename F>
    Observer<D> Observe(F&& f) const
    {
        return REACT::Observe(*this, std::forward<F>(f));
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
protected:
    using BaseT     = REACT_IMPL::EventStreamBase<D,std::reference_wrapper<E>>;

private:
    using NodeT     = REACT_IMPL::EventStreamNode<D,std::reference_wrapper<E>>;
    using NodePtrT  = REACT_IMPL::SharedPtrT<NodeT>;

public:
    using ValueT = E;

    Events() = default;
    Events(const Events&) = default;

    Events(Events&& other) :
        EventStreamBase{ std::move(other) }
    {}

    explicit Events(NodePtrT&& nodePtr) :
        EventStreamBase{ std::move(nodePtr) }
    {}

    bool Equals(const Events& other) const
    {
        return BaseT::Equals(other);
    }

    bool IsValid() const
    {
        return BaseT::IsValid();
    }

    auto Forward() const
        -> decltype(REACT::Forward(std::declval<Events>()))
    {
        return REACT::Forward(*this);
    }

    template <typename ... TArgs>
    auto Merge(const Events<D,TArgs>& ... args)
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

    template <typename F>
    Observer<D> Observe(F&& f) const
    {
        return REACT::Observe(*this, std::forward<F>(f));
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventSource
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename E = EventToken
>
class EventSource : public Events<D,E>
{
private:
    using NodeT     = REACT_IMPL::EventSourceNode<D,E>;
    using NodePtrT  = REACT_IMPL::SharedPtrT<NodeT>;

public:
    EventSource() = default;
    EventSource(const EventSource&) = default;

    EventSource(EventSource&& other) :
        Events{ std::move(other) }
    {}

    explicit EventSource(NodePtrT&& nodePtr) :
        Events{ std::move(nodePtr) }
    {}

    void Emit(const E& e) const
    {
        BaseT::emit(e);
    }

    void Emit(E&& e) const
    {
        BaseT::emit(std::move(e));
    }

    template <class = std::enable_if<std::is_same<E,EventToken>::value>::type>
    void Emit() const
    {
        BaseT::emit(EventToken::token);
    }

    const EventSource& operator<<(const E& e) const
    {
        BaseT::emit(e);
        return *this;
    }

    const EventSource& operator<<(E&& e) const
    {
        BaseT::emit(std::move(e));
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
    using NodePtrT  = REACT_IMPL::SharedPtrT<NodeT>;

public:
    EventSource() = default;
    EventSource(const EventSource&) = default;

    EventSource(EventSource&& other) :
        Events{ std::move(other) }
    {}

    explicit EventSource(NodePtrT&& nodePtr) :
        Events{ std::move(nodePtr) }
    {}

    void Emit(std::reference_wrapper<E> e) const
    {
        BaseT::emit(e);
    }

    const EventSource& operator<<(std::reference_wrapper<E> e) const
    {
        BaseT::emit(e);
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
    using NodePtrT  = REACT_IMPL::SharedPtrT<NodeT>;

public:    
    TempEvents() = default;
    TempEvents(const TempEvents&) = default;

    TempEvents(TempEvents&& other) :
        Events{ std::move(other) }
    {}

    explicit TempEvents(NodePtrT&& nodePtr) :
        Events{ std::move(nodePtr) }
    {}

    TOp StealOp()
    {
        return std::move(reinterpret_cast<NodeT*>(ptr_.get())->StealOp());
    }

    template <typename ... TArgs>
    auto Merge(const Events<D,TArgs>& ... args)
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

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MakeEventSource
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename E>
auto MakeEventSource()
    -> EventSource<D,E>
{
    return EventSource<D,E>(
        std::make_shared<REACT_IMPL::EventSourceNode<D,E>>());
}

template <typename D>
auto MakeEventSource()
    -> EventSource<D,EventToken>
{
    return EventSource<D,EventToken>(
        std::make_shared<REACT_IMPL::EventSourceNode<D,EventToken>>());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename E
>
auto Forward(const Events<D,E>& other)
    -> Events<D,E>
{
    return Events<D,E>(
        std::make_shared<REACT_IMPL::EventForwardNode<D,E>>(other.NodePtr()));
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
    static_assert(sizeof...(TArgs) > 0,
        "react::Merge requires at least 2 arguments.");

    return TempEvents<D,E,TOp>(
        std::make_shared<REACT_IMPL::EventOpNode<D,E,TOp>>(
            arg1.NodePtr(), args.NodePtr() ...));
}

template
<
    typename TLeftEvents,
    typename TRightEvents,
    typename D = TLeftEvents::DomainT,
    typename TLeftVal = TLeftEvents::ValueT,
    typename TRightVal = TRightEvents::ValueT,
    typename E = TLeftVal,
    typename TOp = REACT_IMPL::EventMergeOp<E,
        REACT_IMPL::EventStreamNodePtrT<D,TLeftVal>,
        REACT_IMPL::EventStreamNodePtrT<D,TRightVal>>,
    class = std::enable_if<
        IsEvent<TLeftEvents>::value>::type,
    class = std::enable_if<
        IsEvent<TRightEvents>::value>::type
>
auto operator|(const TLeftEvents& lhs, const TRightEvents& rhs)
    -> TempEvents<D,E,TOp>
{
    return TempEvents<D,E,TOp>(
        std::make_shared<REACT_IMPL::EventOpNode<D,E,TOp>>(
            lhs.NodePtr(), rhs.NodePtr()));
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
    return TempEvents<D,E,TOp>(
        std::make_shared<REACT_IMPL::EventOpNode<D,E,TOp>>(
            lhs.StealOp(), rhs.StealOp()));
}

template
<
    typename D,
    typename TLeftVal,
    typename TLeftOp,
    typename TRightEvents,
    typename TRightVal = TRightEvents::ValueT,
    typename E = TLeftVal,
    typename TOp = REACT_IMPL::EventMergeOp<E,
        TLeftOp,
        REACT_IMPL::EventStreamNodePtrT<D,TRightVal>>,
    class = std::enable_if<
        IsEvent<TRightEvents>::value>::type
>
auto operator|(TempEvents<D,TLeftVal,TLeftOp>&& lhs, const TRightEvents& rhs)
    -> TempEvents<D,E,TOp>
{
    return TempEvents<D,E,TOp>(
        std::make_shared<REACT_IMPL::EventOpNode<D,E,TOp>>(
            lhs.StealOp(), rhs.NodePtr()));
}

template
<
    typename TLeftEvents,
    typename D,
    typename TRightVal,
    typename TRightOp,
    typename TLeftVal = TLeftEvents::ValueT,
    typename E = TLeftVal,
    typename TOp = REACT_IMPL::EventMergeOp<E,
        REACT_IMPL::EventStreamNodePtrT<D,TRightVal>,
        TRightOp>,
    class = std::enable_if<
        IsEvent<TLeftEvents>::value>::type
>
auto operator|(const TLeftEvents& lhs, TempEvents<D,TRightVal,TRightOp>&& rhs)
    -> TempEvents<D,E,TOp>
{
    return TempEvents<D,E,TOp>(
        std::make_shared<REACT_IMPL::EventOpNode<D,E,TOp>>(
            lhs.NodePtr(), rhs.StealOp()));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Filter
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename E,
    typename FIn,
    typename F = std::decay<FIn>::type,
    typename TOp = REACT_IMPL::EventFilterOp<E,F,
        REACT_IMPL::EventStreamNodePtrT<D,E>>
>
auto Filter(const Events<D,E>& src, FIn&& filter)
    -> TempEvents<D,E,TOp>
{
    return TempEvents<D,E,TOp>(
        std::make_shared<REACT_IMPL::EventOpNode<D,E,TOp>>(
            std::forward<FIn>(filter), src.NodePtr()));
}

template
<
    typename D,
    typename E,
    typename TOpIn,
    typename FIn,
    typename F = std::decay<FIn>::type,
    typename TOpOut = REACT_IMPL::EventFilterOp<E,F,TOpIn>
>
auto Filter(TempEvents<D,E,TOpIn>&& src, FIn&& filter)
    -> TempEvents<D,E,TOpOut>
{
    return TempEvents<D,E,TOpOut>(
        std::make_shared<REACT_IMPL::EventOpNode<D,E,TOpOut>>(
            std::forward<FIn>(filter), src.StealOp()));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Transform
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename EIn,
    typename FIn,
    typename F = std::decay<FIn>::type,
    typename EOut = std::result_of<F(EIn)>::type,
    typename TOp = REACT_IMPL::EventTransformOp<EIn,F,
        REACT_IMPL::EventStreamNodePtrT<D,EIn>>
>
auto Transform(const Events<D,EIn>& src, FIn&& func)
    -> TempEvents<D,EOut,TOp>
{
    return TempEvents<D,EOut,TOp>(
        std::make_shared<REACT_IMPL::EventOpNode<D,EOut,TOp>>(
            std::forward<FIn>(func), src.NodePtr()));
}

template
<
    typename D,
    typename EIn,
    typename TOpIn,
    typename FIn,
    typename F = std::decay<FIn>::type,
    typename EOut = std::result_of<F(EIn)>::type,
    typename TOpOut = REACT_IMPL::EventTransformOp<EIn,F,TOpIn>
>
auto Transform(TempEvents<D,EIn,TOpIn>&& src, FIn&& func)
    -> TempEvents<D,EOut,TOpOut>
{
    return TempEvents<D,EOut,TOpOut>(
        std::make_shared<REACT_IMPL::EventOpNode<D,EOut,TOpOut>>(
            std::forward<FIn>(func), src.StealOp()));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Flatten
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename TInnerValue
>
auto Flatten(const Signal<D,Events<D,TInnerValue>>& node)
    -> Events<D,TInnerValue>
{
    return Events<D,TInnerValue>(
        std::make_shared<REACT_IMPL::EventFlattenNode<D, Events<D,TInnerValue>, TInnerValue>>(
            node.NodePtr(), node().NodePtr()));
}

/******************************************/ REACT_END /******************************************/
