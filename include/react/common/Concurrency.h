
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_COMMON_CONCURRENCY_H_INCLUDED
#define REACT_COMMON_CONCURRENCY_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <condition_variable>
#include <mutex>

/***************************************/ REACT_IMPL_BEGIN /**************************************/

class BlockingCondition
{
public:
    inline void Block()
    {// mutex_
        std::lock_guard<std::mutex> scopedLock(mutex_);
        blocked_ = true;
    }// ~mutex_

    inline void Unblock()
    {// mutex_
        std::lock_guard<std::mutex> scopedLock(mutex_);
        blocked_ = false;
        condition_.notify_all();
    }// ~mutex_

    inline void WaitForUnblock()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return !blocked_; });
    }

    inline bool IsBlocked()
    {// mutex_
        std::lock_guard<std::mutex> scopedLock(mutex_);
        return blocked_;
    }// ~mutex_

    template <typename F>
    auto Run(F&& func) -> decltype(func(false))
    {// mutex_
        std::lock_guard<std::mutex> scopedLock(mutex_);
        return func(blocked_);
    }// ~mutex_

    template <typename F>
    bool RunIfBlocked(F&& func)
    {// mutex_
        std::lock_guard<std::mutex> scopedLock(mutex_);

        if (!blocked_)
            return false;

        func();
        return true;            
    }// ~mutex_

    template <typename F>
    bool RunIfUnblocked(F&& func)
    {// mutex_
        std::lock_guard<std::mutex> scopedLock(mutex_);

        if (blocked_)
            return false;

        func();
        return true;            
    }// ~mutex_

private:
    std::mutex                  mutex_;
    std::condition_variable     condition_;

    bool    blocked_ = false;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ConditionalCriticalSection
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TMutex, bool is_enabled>
class ConditionalCriticalSection;

template <typename TMutex>
class ConditionalCriticalSection<TMutex,false>
{
public:
    template <typename F>
    void Access(const F& f) { f(); }
};

template <typename TMutex>
class ConditionalCriticalSection<TMutex,true>
{
public:
    template <typename F>
    void Access(const F& f)
    {// mutex_
        std::lock_guard<TMutex> lock(mutex_);

        f();
    }// ~mutex_
private:
    TMutex  mutex_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_COMMON_CONCURRENCY_H_INCLUDED