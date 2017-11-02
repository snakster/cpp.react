
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_COMMON_PTR_CACHE_H_INCLUDED
#define REACT_COMMON_PTR_CACHE_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <memory>
#include <mutex>
#include <unordered_map>

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// A cache to objects of type shared_ptr<V> that stores weak pointers.
/// Thread-safe.
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename K, typename V>
class WeakPtrCache
{
public:
    /// Returns a shared pointer to an object that existings in the cache, indexed by key.
    /// If no hit was found, createFunc is used to construct the object managed by shared pointer.
    /// A weak pointer to the result is stored in the cache.
    template <typename F>
    std::shared_ptr<V> LookupOrCreate(const K& key, F&& createFunc)
    {
        std::lock_guard<std::mutex> scopedLock(mutex_);

        auto it = map_.find(key);
        if (it != map_.end())
        {
            // Lock fails, if the object was cached before, but has been released already.
            // In that case we re-create it.
            if (auto ptr = it->second.lock())
            {
                return ptr;
            }
        }

        std::shared_ptr<V> v = createFunc();
        auto res = map_.emplace(key, v);
        return v;
    }

    /// Removes an entry from the cache.
    void Erase(const K& key)
    {
        std::lock_guard<std::mutex> scopedLock(mutex_);

        auto it = map_.find(key);
        if (it != map_.end())
        {
            map_.erase(it);
        }
    }

private:
    std::mutex mutex_;
    std::unordered_map<K, std::weak_ptr<V>> map_;
};


/******************************************/ REACT_END /******************************************/

#endif // REACT_COMMON_PTR_CACHE_H_INCLUDED