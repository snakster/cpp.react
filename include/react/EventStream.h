
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/Defs.h"

#include <thread>
#include <type_traits>
#include <vector>

#include "ReactiveBase.h"
#include "ReactiveDomain.h"
#include "Observer.h"
#include "react/graph/EventStreamNodes.h"

/*****************************************/ REACT_BEGIN /*****************************************/

enum class EventToken { token };

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Events
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename E = EventToken
>
class Events : public Reactive<REACT_IMPL::EventStreamNode<D,E>>
{
protected:
    using NodeT = REACT_IMPL::EventStreamNode<D, E>;

public:
    using ValueT = E;

    Events() :
        Reactive()
    {
    }

    explicit Events(const std::shared_ptr<NodeT>& ptr) :
        Reactive(ptr)
    {
    }

    template <typename F>
    Events Filter(F&& f)
    {
        return react::Filter(*this, std::forward<F>(f));
    }

    template <typename F>
    Events Transform(F&& f)
    {
        return react::Transform(*this, std::forward<F>(f));
    }

    template <typename F>
    Observer<D> Observe(F&& f)
    {
        return react::Observe(*this, std::forward<F>(f));
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
/// Eventsource
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename E = EventToken
>
class EventSource : public Events<D,E>
{
private:
    using NodeT = REACT_IMPL::EventSourceNode<D, E>;

public:
    EventSource() :
        Events()
    {
    }

    explicit EventSource(const std::shared_ptr<NodeT>& ptr) :
        Events(ptr)
    {
    }

    template <typename V>
    void Emit(V&& v) const
    {
        D::AddInput(*std::static_pointer_cast<NodeT>(ptr_), std::forward<V>(v));
    }

    template <typename = std::enable_if<std::is_same<E,EventToken>::value>::type>
    void Emit() const
    {
        Emit(EventToken::token);
    }

    const EventSource& operator<<(const E& e) const
    {
        Emit(e);
        return *this;
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MakeEventSource
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename E>
auto MakeEventSource()
    -> EventSource<D,E>
{
    return EventSource<D,E>(
        std::make_shared<REACT_IMPL::EventSourceNode<D,E>>(false));
}

template <typename D>
auto MakeEventSource()
    -> EventSource<D,EventToken>
{
    return EventSource<D,EventToken>(
        std::make_shared<REACT_IMPL::EventSourceNode<D,EventToken>>(false));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Merge
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename TArg1,
    typename ... TArgs
>
inline auto Merge(const Events<D,TArg1>& arg1,
                  const Events<D,TArgs>& ... args)
    -> Events<D,TArg1>
{
    static_assert(sizeof...(TArgs) > 0,
        "react::Merge requires at least 2 arguments.");

    typedef TArg1 E;
    return Events<D,E>(
        std::make_shared<REACT_IMPL::EventMergeNode<D, E, TArg1, TArgs ...>>(
            arg1.GetPtr(), args.GetPtr() ..., false));
}

template
<
    typename D,
    typename TLeftArg,
    typename TRightArg
>
inline auto operator|(const Events<D,TLeftArg>& lhs,
                      const Events<D,TRightArg>& rhs)
    -> Events<D, TLeftArg>
{
    return Merge(lhs,rhs);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Filter
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename E,
    typename F
>
inline auto Filter(const Events<D,E>& src, F&& filter)
    -> Events<D,E>
{
    return Events<D,E>(
        std::make_shared<REACT_IMPL::EventFilterNode<D, E>>(
            src.GetPtr(), std::forward<F>(filter), false));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Transform
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename TIn,
    typename F
>
inline auto Transform(const Events<D,TIn>& src, F&& func)
    -> Events<D, typename std::result_of<F(TIn)>::type>
{
    using TOut = typename std::result_of<F(TIn)>::type;

    return Events<D,TOut>(
        std::make_shared<REACT_IMPL::EventTransformNode<D, TIn, TOut>>(
            src.GetPtr(), std::forward<F>(func), false));
}

/******************************************/ REACT_END /******************************************/
