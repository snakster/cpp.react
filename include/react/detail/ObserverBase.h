
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/detail/Defs.h"

#include <atomic>
#include <memory>
#include <unordered_map>
#include <utility>

#include "IReactiveNode.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

template <typename D>
class Observable;

class IObserver;

// tbb tasks are non-preemptible, thread local flag for each worker
namespace current_observer_state_
{
    static REACT_TLS bool    shouldDetach = false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ObserverRegistry
///////////////////////////////////////////////////////////////////////////////////////////////////
// Todo: still ugly, needs more refactoring
template <typename D>
class ObserverRegistry
{
private:
    class Entry
    {
    public:
        Entry(std::unique_ptr<IObserver>&& nodePtr, const Observable<D>* subjectPtr) :
            nodePtr_{ std::move(nodePtr) },
            SubjectPtr{ subjectPtr }
        {}

        Entry(const Entry&) = delete;

        Entry(Entry&& other) :
            nodePtr_{ std::move(other.nodePtr_) },
            SubjectPtr{ other.SubjectPtr }
        {}

        const Observable<D>* SubjectPtr;

    private:
        // Manage lifetime
        std::unique_ptr<IObserver> nodePtr_;
    };

public:
    // Pass ownership of obs node to registry
    IObserver* Register(std::unique_ptr<IObserver>&& obsPtr, const Observable<D>* subjectPtr)
    {
        // Use raw ptr copy as index to find owned version of itself
        auto* rawObsPtr = obsPtr.get();
        
        observerMap_.emplace(rawObsPtr, Entry{ std::move(obsPtr), subjectPtr });

        return rawObsPtr;
    }

    void Unregister(IObserver* obsPtr)
    {
        obsPtr->detachObserver();
        observerMap_.erase(obsPtr);
    }

    void UnregisterFrom(const Observable<D>* subjectPtr)
    {
        auto it = observerMap_.begin();
        while (it != observerMap_.end())
        {
            if (it->second.SubjectPtr == subjectPtr)
            {
                it->first->detachObserver();
                it = observerMap_.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

private:
    std::unordered_map<IObserver*,Entry>   observerMap_;
};

template <typename D>
class DomainSpecificObserverRegistry
{
public:
    DomainSpecificObserverRegistry() = delete;

    static ObserverRegistry<D>& Instance()
    {
        static ObserverRegistry<D> instance;
        return instance;
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Observable
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class Observable
{
public:
    void    IncObsCount()       { obsCount_.fetch_add(1, std::memory_order_relaxed); }
    void    DecObsCount()       { obsCount_.fetch_sub(1, std::memory_order_relaxed); }
    uint    GetObsCount() const { return obsCount_.load(std::memory_order_relaxed); }

    ~Observable()
    {
        if (GetObsCount() > 0)
            DomainSpecificObserverRegistry<D>::Instance().UnregisterFrom(this);
    }

private:
    std::atomic<uint>   obsCount_   = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IObserver
///////////////////////////////////////////////////////////////////////////////////////////////////
class IObserver
{
public:
    virtual ~IObserver() = default;

private:
    virtual void detachObserver() = 0;

    template <typename D>
    friend class ObserverRegistry;
};

/****************************************/ REACT_IMPL_END /***************************************/