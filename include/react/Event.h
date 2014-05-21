
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
#include "react/common/Util.h"
#include "react/detail/EventBase.h"

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward declarations
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename S>
class Signal;

template <typename D, typename ... TValues>
class SignalPack;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Token
///////////////////////////////////////////////////////////////////////////////////////////////////
enum class Token { value };

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

    // Explicit emit
    void Emit(const E& e) const         { BaseT::emit(e); }
    void Emit(E&& e) const              { BaseT::emit(std::move(e)); }

    template <class = std::enable_if<std::is_same<E,Token>::value>::type>
    void Emit() const   { BaseT::emit(Token::value); }

    // Function object style
    void operator()(const E& e) const   { BaseT::emit(e); }
    void operator()(E&& e) const        { BaseT::emit(std::move(e)); }

    template <class = std::enable_if<std::is_same<E,Token>::value>::type>
    void operator()() const { BaseT::emit(Token::value); }

    // Stream style
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

    // Explicit emit
    void Emit(std::reference_wrapper<E> e) const        { BaseT::emit(e); }

    // Function object style
    void operator()(std::reference_wrapper<E> e) const  { BaseT::emit(e); }

    // Stream style
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

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MakeEventSource
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename E>
auto MakeEventSource()
    -> EventSource<D,E>
{
    using REACT_IMPL::EventSourceNode;

    return EventSource<D,E>(
        std::make_shared<EventSourceNode<D,E>>());
}

template <typename D>
auto MakeEventSource()
    -> EventSource<D,Token>
{
    using REACT_IMPL::EventSourceNode;

    return EventSource<D,Token>(
        std::make_shared<EventSourceNode<D,Token>>());
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
        "react::Merge requires at least 2 arguments.");

    return TempEvents<D,E,TOp>(
        std::make_shared<EventOpNode<D,E,TOp>>(
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
    using REACT_IMPL::EventOpNode;

    return TempEvents<D,E,TOp>(
        std::make_shared<EventOpNode<D,E,TOp>>(
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
    using REACT_IMPL::EventOpNode;

    return TempEvents<D,E,TOp>(
        std::make_shared<EventOpNode<D,E,TOp>>(
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
    using REACT_IMPL::EventOpNode;

    return TempEvents<D,E,TOp>(
        std::make_shared<EventOpNode<D,E,TOp>>(
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
    using REACT_IMPL::EventOpNode;

    return TempEvents<D,E,TOp>(
        std::make_shared<EventOpNode<D,E,TOp>>(
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

    using F = std::decay<FIn>::type;

    struct NodeBuilder_
    {
        NodeBuilder_(const Events<D,E>& source, FIn&& func) :
            MySource{ source },
            MyFunc{ std::forward<FIn>(func) }
        {}

        auto operator()(const Signal<D,TDepValues>& ... deps)
            -> Events<D,E>
        {
            return Events<D,E>(
                std::make_shared<SyncedEventFilterNode<D,E,F,TDepValues ...>>(
                     MySource.NodePtr(), std::forward<FIn>(MyFunc), deps.NodePtr() ...));
        }

        const Events<D,E>&      MySource;
        FIn                     MyFunc;
    };

    return REACT_IMPL::apply(
        NodeBuilder_{ source, std::forward<FIn>(func) },
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
    typename F = std::decay<FIn>::type,
    typename TOut = std::result_of<F(TIn)>::type,
    typename TOp = REACT_IMPL::EventTransformOp<TIn,F,
        REACT_IMPL::EventStreamNodePtrT<D,TIn>>
>
auto Transform(const Events<D,TIn>& src, FIn&& func)
    -> TempEvents<D,TOut,TOp>
{
    using REACT_IMPL::EventOpNode;

    return TempEvents<D,TOut,TOp>(
        std::make_shared<EventOpNode<D,TOut,TOp>>(
            std::forward<FIn>(func), src.NodePtr()));
}

template
<
    typename D,
    typename TIn,
    typename TOpIn,
    typename FIn,
    typename F = std::decay<FIn>::type,
    typename TOut = std::result_of<F(TIn)>::type,
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
    typename TOut = std::result_of<FIn(TIn,TDepValues...)>::type
>
auto Transform(const Events<D,TIn>& source, const SignalPack<D,TDepValues...>& depPack, FIn&& func)
    -> Events<D,TOut>
{
    using REACT_IMPL::SyncedEventTransformNode;

    using F = std::decay<FIn>::type;

    struct NodeBuilder_
    {
        NodeBuilder_(const Events<D,TIn>& source, FIn&& func) :
            MySource{ source },
            MyFunc{ std::forward<FIn>(func) }
        {}

        auto operator()(const Signal<D,TDepValues>& ... deps)
            -> Events<D,TOut>
        {
            return Events<D,TOut>(
                std::make_shared<SyncedEventTransformNode<D,TIn,TOut,F,TDepValues ...>>(
                     MySource.NodePtr(), std::forward<FIn>(MyFunc), deps.NodePtr() ...));
        }

        const Events<D,TIn>&    MySource;
        FIn                     MyFunc;
    };

    return REACT_IMPL::apply(
        NodeBuilder_{ source, std::forward<FIn>(func) },
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
auto Flatten(const Signal<D,Events<D,TInnerValue>>& node)
    -> Events<D,TInnerValue>
{
    return Events<D,TInnerValue>(
        std::make_shared<REACT_IMPL::EventFlattenNode<D, Events<D,TInnerValue>, TInnerValue>>(
            node.NodePtr(), node().NodePtr()));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Tokenize
///////////////////////////////////////////////////////////////////////////////////////////////////
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

/******************************************/ REACT_END /******************************************/
