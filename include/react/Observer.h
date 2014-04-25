
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/detail/Defs.h"

#include "react/detail/IReactiveNode.h"
#include "react/detail/ObserverBase.h"
#include "react/detail/graph/ObserverNodes.h"

/*****************************************/ REACT_BEGIN /*****************************************/

template <typename T>
class Reactive;

template <typename D, typename S>
class Signal;

template <typename D, typename E>
class Events;

enum class EventToken;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Observer
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class Observer
{
public:
    using SubjectT      = REACT_IMPL::NodeBase<D>;
    using ObserverNodeT = REACT_IMPL::IObserverNode;

    Observer() :
        ptr_{ nullptr },
        subject_{ nullptr }
    {}

    Observer(ObserverNodeT* ptr, const std::shared_ptr<SubjectT>& subject) :
        ptr_{ ptr },
        subject_{ subject }
    {}

    const ObserverNodeT* GetPtr() const
    {
        return ptr_;
    }

    bool Detach()
    {
        if (ptr_ == nullptr)
            return false;

        REACT_IMPL::DomainSpecificObserverRegistry<D>::Instance().Unregister(ptr_);
        return true;
    }

private:
    // Ownership managed by registry
    ObserverNodeT*  ptr_;

    // While the observer handle exists, the subject is not destroyed
    std::shared_ptr<SubjectT>    subject_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Observe
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename FIn,
    typename TArg
>
auto Observe(const Signal<D,TArg>& subject, FIn&& func)
    -> Observer<D>
{
    using F = std::decay<FIn>::type;
    using TNode = REACT_IMPL::SignalObserverNode<D,TArg,F>;

    auto* raw = REACT_IMPL::DomainSpecificObserverRegistry<D>::Instance().
        template Register<TNode>(subject, std::forward<FIn>(func));

    return Observer<D>(raw, subject.GetPtr());
}

template
<
    typename D,
    typename FIn,
    typename TArg,
    typename = std::enable_if<
        ! std::is_same<TArg,EventToken>::value>::type
>
auto Observe(const Events<D,TArg>& subject, FIn&& func)
    -> Observer<D>
{
    using F = std::decay<FIn>::type;
    using TNode = REACT_IMPL::EventObserverNode<D,TArg,F>;

    auto* raw = REACT_IMPL::DomainSpecificObserverRegistry<D>::Instance().
        template Register<TNode>(subject, std::forward<FIn>(func));

    return Observer<D>(raw, subject.GetPtr());
}

template
<
    typename D,
    typename FIn
>
auto Observe(const Events<D,EventToken>& subject, FIn&& func)
    -> Observer<D>
{
    auto wrapper = [func] (EventToken _) { func(); };

    using TNode = REACT_IMPL::EventObserverNode<D,EventToken,decltype(wrapper)>;

    auto* raw = REACT_IMPL::DomainSpecificObserverRegistry<D>::Instance().
        template Register<TNode>(subject, std::move(wrapper));

    return Observer<D>(raw, subject.GetPtr());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// DetachAllObservers
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    template <typename Domain_, typename Val_> class TNode,
    typename TArg
>
void DetachAllObservers(const Reactive<TNode<D,TArg>>& subject)
{
    REACT_IMPL::DomainSpecificObserverRegistry<D>::Instance().UnregisterFrom(
        subject.GetPtr().get());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// DetachThisObserver
///////////////////////////////////////////////////////////////////////////////////////////////////
inline void DetachThisObserver()
{
    REACT_IMPL::current_observer_state_::shouldDetach = true;
}

/******************************************/ REACT_END /******************************************/
