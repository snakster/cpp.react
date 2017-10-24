
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
/// Stores weak pointers to V indexed by K.
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename K, typename V>
class WeakPtrCache
{
public:
    template <typename F>
    std::shared_ptr<V> LookupOrCreate(const K& key, F&& createFunc)
    {
        std::lock_guard<std::mutex> scopedLock(mutex_);

        auto it = map_.find(key);
        if (it != map_.end())
        {
            if (auto ptr = it->second.lock())
            {
                return ptr;
            }
        }

        std::shared_ptr<V> v = createFunc();
        auto res = map_.emplace(key, v);
        return v;
    }

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

#endif // REACT_COMMON_INDEXED_STORAGE_H_INCLUDED