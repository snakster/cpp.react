
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_OBSERVER_H_INCLUDED
#define REACT_OBSERVER_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <memory>
#include <utility>

#include "react/common/Util.h"
#include "react/detail/IReactiveNode.h"
#include "react/detail/ObserverBase.h"
#include "react/detail/graph/ObserverNodes.h"

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward declarations
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename S>
class Signal;

template <typename D, typename ... TValues>
class SignalPack;

template <typename D, typename E>
class Events;

using REACT_IMPL::ObserverAction;
using REACT_IMPL::WeightHint;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Observer
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class Observer
{
private:
    using SubjectPtrT   = std::shared_ptr<REACT_IMPL::ObservableNode<D>>;
    using NodeT         = REACT_IMPL::ObserverNode<D>;

public:
    // Default ctor
    Observer() :
        nodePtr_( nullptr ),
        subjectPtr_( nullptr )
    {}

    // Move ctor
    Observer(Observer&& other) :
        nodePtr_( other.nodePtr_ ),
        subjectPtr_( std::move(other.subjectPtr_) )
    {
        other.nodePtr_ = nullptr;
        other.subjectPtr_.reset();
    }

    // Node ctor
    Observer(NodeT* nodePtr, const SubjectPtrT& subjectPtr) :
        nodePtr_( nodePtr ),
        subjectPtr_( subjectPtr )
    {}

    // Move assignment
    Observer& operator=(Observer&& other)
    {
        nodePtr_ = other.nodePtr_;
        subjectPtr_ = std::move(other.subjectPtr_);

        other.nodePtr_ = nullptr;
        other.subjectPtr_.reset();

        return *this;
    }

    // Deleted copy ctor and assignment
    Observer(const Observer&) = delete;
    Observer& operator=(const Observer&) = delete;

    void Detach()
    {
        assert(IsValid());
        subjectPtr_->UnregisterObserver(nodePtr_);
    }

    bool IsValid() const
    {
        return nodePtr_ != nullptr;
    }

    void SetWeightHint(WeightHint weight)
    {
        assert(IsValid());
        nodePtr_->SetWeightHint(weight);
    }

private:
    // Owned by subject
    NodeT*          nodePtr_;

    // While the observer handle exists, the subject is not destroyed
    SubjectPtrT     subjectPtr_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ScopedObserver
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class ScopedObserver
{
public:
    // Move ctor
    ScopedObserver(ScopedObserver&& other) :
        obs_( std::move(other.obs_) )
    {}

    // Construct from observer
    ScopedObserver(Observer<D>&& obs) :
        obs_( std::move(obs) )
    {}

    // Move assignment
    ScopedObserver& operator=(ScopedObserver&& other)
    {
        obs_ = std::move(other.obs_);
    }

    // Deleted default ctor, copy ctor and assignment
    ScopedObserver() = delete;
    ScopedObserver(const ScopedObserver&) = delete;
    ScopedObserver& operator=(const ScopedObserver&) = delete;

    ~ScopedObserver()
    {
        obs_.Detach();
    }

    bool IsValid() const
    {
        return obs_.IsValid();
    }

    void SetWeightHint(WeightHint weight)
    {
        obs_.SetWeightHint(weight);
    }

private:
    Observer<D>     obs_;    
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Observe - Signals
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename FIn,
    typename S
>
auto Observe(const Signal<D,S>& subject, FIn&& func)
    -> Observer<D>
{
    using REACT_IMPL::IObserver;
    using REACT_IMPL::ObserverNode;
    using REACT_IMPL::SignalObserverNode;
    using REACT_IMPL::AddDefaultReturnValueWrapper;

    using F = typename std::decay<FIn>::type;
    using R = typename std::result_of<FIn(S)>::type;
    using WrapperT = AddDefaultReturnValueWrapper<F,ObserverAction,ObserverAction::next>;

    // If return value of passed function is void, add ObserverAction::next as
    // default return value.
    using NodeT = typename std::conditional<
        std::is_same<void,R>::value,
        SignalObserverNode<D,S,WrapperT>,
        SignalObserverNode<D,S,F>
            >::type;

    const auto& subjectPtr = GetNodePtr(subject);

    std::unique_ptr<ObserverNode<D>> nodePtr( new NodeT(subjectPtr, std::forward<FIn>(func)) );
    ObserverNode<D>* rawNodePtr = nodePtr.get();

    subjectPtr->RegisterObserver(std::move(nodePtr));

    return Observer<D>( rawNodePtr, subjectPtr );
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Observe - Events
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename FIn,
    typename E
>
auto Observe(const Events<D,E>& subject, FIn&& func)
    -> Observer<D>
{
    using REACT_IMPL::IObserver;
    using REACT_IMPL::ObserverNode;
    using REACT_IMPL::EventObserverNode;
    using REACT_IMPL::AddDefaultReturnValueWrapper;
    using REACT_IMPL::AddObserverRangeWrapper;
    using REACT_IMPL::IsCallableWith;
    using REACT_IMPL::EventRange;

    using F = typename std::decay<FIn>::type;

    using WrapperT =
        typename std::conditional<
            IsCallableWith<F, ObserverAction, EventRange<E>>::value,
            F,
            typename std::conditional<
                IsCallableWith<F, ObserverAction, E>::value,
                AddObserverRangeWrapper<E, F>,
                typename std::conditional<
                    IsCallableWith<F, void, EventRange<E>>::value,
                    AddDefaultReturnValueWrapper<F,ObserverAction,ObserverAction::next>,
                    typename std::conditional<
                        IsCallableWith<F, void, E>::value,
                        AddObserverRangeWrapper<E,
                            AddDefaultReturnValueWrapper<F,ObserverAction,ObserverAction::next>>,
                        void
                    >::type
                >::type
            >::type
        >::type;

    static_assert(
        ! std::is_same<WrapperT,void>::value,
        "Observe: Passed function does not match any of the supported signatures.");
    
    using NodeT = EventObserverNode<D,E,WrapperT>;
    
    const auto& subjectPtr = GetNodePtr(subject);

    std::unique_ptr<ObserverNode<D>> nodePtr( new NodeT(subjectPtr, std::forward<FIn>(func)) );
    ObserverNode<D>* rawNodePtr = nodePtr.get();

    subjectPtr->RegisterObserver(std::move(nodePtr));

    return Observer<D>( rawNodePtr, subjectPtr );
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Observe - Synced
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename FIn,
    typename E,
    typename ... TDepValues
>
auto Observe(const Events<D,E>& subject,
             const SignalPack<D,TDepValues...>& depPack, FIn&& func)
    -> Observer<D>
{
    using REACT_IMPL::IObserver;
    using REACT_IMPL::ObserverNode;
    using REACT_IMPL::SyncedObserverNode;
    using REACT_IMPL::AddDefaultReturnValueWrapper;
    using REACT_IMPL::AddObserverRangeWrapper;
    using REACT_IMPL::IsCallableWith;
    using REACT_IMPL::EventRange;

    using F = typename std::decay<FIn>::type;

    using WrapperT =
        typename std::conditional<
            IsCallableWith<F, ObserverAction, EventRange<E>, TDepValues ...>::value,
            F,
            typename std::conditional<
                IsCallableWith<F, ObserverAction, E, TDepValues ...>::value,
                AddObserverRangeWrapper<E, F, TDepValues ...>,
                typename std::conditional<
                    IsCallableWith<F, void, EventRange<E>, TDepValues ...>::value,
                    AddDefaultReturnValueWrapper<F, ObserverAction ,ObserverAction::next>,
                    typename std::conditional<
                        IsCallableWith<F, void, E, TDepValues ...>::value,
                        AddObserverRangeWrapper<E,
                            AddDefaultReturnValueWrapper<F,ObserverAction,ObserverAction::next>,
                                TDepValues...>,
                            void
                        >::type
                >::type
            >::type
        >::type;

    static_assert(
        ! std::is_same<WrapperT,void>::value,
        "Observe: Passed function does not match any of the supported signatures.");

    using NodeT = SyncedObserverNode<D,E,WrapperT,TDepValues ...>;

    struct NodeBuilder_
    {
        NodeBuilder_(const Events<D,E>& subject, FIn&& func) :
            MySubject( subject ),
            MyFunc( std::forward<FIn>(func) )
        {}

        auto operator()(const Signal<D,TDepValues>& ... deps)
            -> ObserverNode<D>*
        {
            return new NodeT(
                GetNodePtr(MySubject), std::forward<FIn>(MyFunc), GetNodePtr(deps) ... );
        }

        const Events<D,E>& MySubject;
        FIn MyFunc;
    };

    const auto& subjectPtr = GetNodePtr(subject);

    std::unique_ptr<ObserverNode<D>> nodePtr( REACT_IMPL::apply(
        NodeBuilder_( subject, std::forward<FIn>(func) ),
        depPack.Data) );

    ObserverNode<D>* rawNodePtr = nodePtr.get();

    subjectPtr->RegisterObserver(std::move(nodePtr));

    return Observer<D>( rawNodePtr, subjectPtr );
}

/******************************************/ REACT_END /******************************************/

#endif // REACT_OBSERVER_H_INCLUDED