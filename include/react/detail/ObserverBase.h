
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_OBSERVERBASE_H_INCLUDED
#define REACT_DETAIL_OBSERVERBASE_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <memory>
#include <vector>
#include <utility>

#include "IReactiveNode.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IObserver
///////////////////////////////////////////////////////////////////////////////////////////////////
class IObserver
{
public:
    virtual ~IObserver() {}

    virtual void UnregisterSelf() = 0;

private:
    virtual void detachObserver() = 0;

    template <typename D>
    friend class Observable;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Observable
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class Observable
{
public:
    Observable() = default;

    ~Observable()
    {
        for (const auto& p : observers_)
            if (p != nullptr)
                p->detachObserver();
    }

    void RegisterObserver(std::unique_ptr<IObserver>&& obsPtr)
    {
        observers_.push_back(std::move(obsPtr));
    }

    void UnregisterObserver(IObserver* rawObsPtr)
    {
        for (auto it = observers_.begin(); it != observers_.end(); ++it)
        {
            if (it->get() == rawObsPtr)
            {
                it->get()->detachObserver();
                observers_.erase(it);
                break;
            }
        }
    }

private:
    std::vector<std::unique_ptr<IObserver>> observers_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_OBSERVERBASE_H_INCLUDED