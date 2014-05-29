
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_COMMON_SOURCEIDSET_H_INCLUDED
#define REACT_COMMON_SOURCEIDSET_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <algorithm>
#include <vector>

#include "tbb/queuing_mutex.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SourceIdSet
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
class SourceIdSet
{
private:
    using MutexT    = tbb::queuing_mutex;
    using DataT     = std::vector<T>;

public:
    void Insert(const T& e)
    {
        MutexT::scoped_lock lock(mutex_);

        data_.push_back(e);
        
        isSorted_ = false;
    }

    void Insert(SourceIdSet<T>& other)
    {
        MutexT::scoped_lock myLock(mutex_);
        MutexT::scoped_lock otherLock(other.mutex_);

        sort();
        other.sort();

        auto l = data_.begin();
        auto r = data_.end();
        auto offset = std::distance(l,r);

        // For each element in other, check if it's already contained in this
        // if not, add it
        for (const auto& e : other.data_)
        {
            l = std::lower_bound(l, r, e);

            // Already in the set?
            if (l < r && *l == e)
                continue;

            auto d = std::distance(data_.begin(), l);

            data_.push_back(e);

            // push_back invalidates iterators
            l = data_.begin() + d;
            r = data_.begin() + offset; 
        }

        std::inplace_merge(data_.begin(), data_.begin() + offset, data_.end());
    }

    void Erase(const T& e)
    {
        MutexT::scoped_lock lock(mutex_);

        data_.erase(std::find(data_.begin(), data_.end(), e));
    }

    void Clear()
    {
        MutexT::scoped_lock lock(mutex_);

        data_.clear();
        isSorted_ = true;
    }

    bool IntersectsWith(SourceIdSet<T>& other)
    {
        MutexT::scoped_lock myLock(mutex_);
        MutexT::scoped_lock otherLock(other.mutex_);

        sort();
        other.sort();

        auto l1 = data_.begin();
        const auto r1 = data_.end();

        auto l2 = other.data_.begin();
        const auto r2 = other.data_.end();

        // Is intersection of turn sourceIds and node sourceIds non-empty?
        while (l1 != r1 && l2 != r2)
        {
            if (*l1 < *l2)
                l1++;
            else if (*l2 < *l1)
                l2++;
            // Equals => Intersect 
            else
                return true;
        }
        return false;
    }

private:
    MutexT  mutex_;
    DataT   data_;
    bool    isSorted_ = false;

    void sort()
    {
        if (isSorted_)
            return;
        
        std::sort(data_.begin(), data_.end());
        isSorted_ = true;
    }
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_COMMON_SOURCEIDSET_H_INCLUDED