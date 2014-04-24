
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/detail/Defs.h"

#include "react/detail/IReactiveNode.h"
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
    using ObserverNodeT = REACT_IMPL::ObserverNode<D>;

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

        D::Observers().Unregister(ptr_);
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
    typename F,
    typename TArg
>
auto Observe(const Signal<D,TArg>& subject, F&& func)
    -> Observer<D>
{
    std::unique_ptr< REACT_IMPL::ObserverNode<D>> pUnique(
        new  REACT_IMPL::SignalObserverNode<D,TArg,F>(
            subject.GetPtr(), std::forward<F>(func)));

    auto* raw = pUnique.get();

    DomainSpecificData<D>::Observers().Register(std::move(pUnique), subject.GetPtr().get());

    return Observer<D>(raw, subject.GetPtr());
}

template
<
    typename D,
    typename F,
    typename TArg,
    typename = std::enable_if<
        ! std::is_same<TArg,EventToken>::value>::type
>
auto Observe(const Events<D,TArg>& subject, F&& func)
    -> Observer<D>
{
    std::unique_ptr< REACT_IMPL::ObserverNode<D>> pUnique(
        new  REACT_IMPL::EventObserverNode<D,TArg,F>(
            subject.GetPtr(), std::forward<F>(func)));

    auto* raw = pUnique.get();

    D::Observers().Register(std::move(pUnique), subject.GetPtr().get());

    return Observer<D>(raw, subject.GetPtr());
}

template
<
    typename D,
    typename F
>
auto Observe(const Events<D,EventToken>& subject, F&& func)
    -> Observer<D>
{
    auto f = [func] (EventToken _) { func(); };
    std::unique_ptr< REACT_IMPL::ObserverNode<D>> pUnique(
        new  REACT_IMPL::EventObserverNode<D,EventToken,decltype(f)>(
            subject.GetPtr(), std::move(f)));

    auto* raw = pUnique.get();

    D::Observers().Register(std::move(pUnique), subject.GetPtr().get());

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
    D::Observers().UnregisterFrom(subject.GetPtr().get());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// DetachThisObserver
///////////////////////////////////////////////////////////////////////////////////////////////////
inline void DetachThisObserver()
{
    REACT_IMPL::current_observer_state_::shouldDetach = true;
}

/******************************************/ REACT_END /******************************************/
