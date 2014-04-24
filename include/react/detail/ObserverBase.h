
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

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ObserverRegistry
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class ObserverRegistry
{
public:
    using SubjectT = NodeBase<D>;

private:
    class Entry
    {
    public:
        Entry(std::unique_ptr<IObserverNode>&& obs, SubjectT* aSubject) :
            obs_{ std::move(obs) },
            Subject{ aSubject }
        {}

        Entry(Entry&& other) :
            obs_(std::move(other.obs_)),
            Subject(other.Subject)
        {}

        SubjectT*    Subject;

    private:
        // Manage lifetime
        std::unique_ptr<IObserverNode>    obs_;
    };

public:
    void Register(std::unique_ptr<IObserverNode>&& obs, SubjectT* subject)
    {
        auto* raw = obs.get();
        observerMap_.emplace(raw, Entry_(std::move(obs),subject));
    }

    void Unregister(IObserverNode* obs)
    {
        obs->Detach();
        observerMap_.erase(obs);
    }

    void UnregisterFrom(SubjectT* subject)
    {
        auto it = observerMap_.begin();
        while (it != observerMap_.end())
        {
            if (it->second.Subject == subject)
            {
                it->first->Detach();
                it = observerMap_.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

private:
    std::unordered_map<IObserverNode*,Entry_>   observerMap_;
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

/****************************************/ REACT_IMPL_END /***************************************/
