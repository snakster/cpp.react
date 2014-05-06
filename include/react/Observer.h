
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

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Observer
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class Observer
{
private:
    using SubjectT  = REACT_IMPL::NodeBasePtrT<D>;
    using NodeT     = REACT_IMPL::IObserver;

public:
    Observer() :
        nodePtr_{ nullptr }
    {}

    Observer(const Observer&) = delete;

    Observer(Observer&& other) :
        nodePtr_{ std::move(other.nodePtr_) },
        subject_{ std::move(other.subject_) }
    {}

    Observer(NodeT* nodePtr, const SubjectT& subject) :
        nodePtr_{ nodePtr },
        subject_{ subject }
    {}

    bool IsValid() const
    {
        return nodePtr_ != nullptr;
    }

    void Detach()
    {
        REACT_ASSERT(IsValid(), "Trying to detach an invalid Observer");
        REACT_IMPL::DomainSpecificObserverRegistry<D>::Instance().Unregister(nodePtr_);
    }

private:
    // Ownership managed by registry
    NodeT*      nodePtr_;

    // While the observer handle exists, the subject is not destroyed
    SubjectT    subject_;    
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ScopedObserver
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class ScopedObserver
{
public:
    ScopedObserver(Observer<D>&& obs) :
        obs_{ std::move(obs) }
    {}

    ~ScopedObserver()
    {
        if (obs_.IsValid())
            obs_.Detach();
    }

private:
    Observer<D>     obs_;    
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// DetachAllObservers
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TSubject>
void DetachAllObservers(const TSubject& subject)
{
    using D = typename TSubject::DomainT;

    REACT_IMPL::DomainSpecificObserverRegistry<D>::Instance().UnregisterFrom(
        subject.NodePtr().get());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// DetachThisObserver
///////////////////////////////////////////////////////////////////////////////////////////////////
inline void DetachThisObserver()
{
    REACT_IMPL::current_observer_state_::shouldDetach = true;
}

/******************************************/ REACT_END /******************************************/