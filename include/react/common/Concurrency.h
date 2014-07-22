
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

#include "react/common/RefCounting.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IWaitingState
///////////////////////////////////////////////////////////////////////////////////////////////////
class IWaitingState
{
public:
    virtual inline ~IWaitingState() {}

    virtual void Wait() = 0;

    virtual void IncWaitCount() = 0;
    virtual void DecWaitCount() = 0;

protected:
    virtual bool IsRefCounted() const = 0;
    virtual void IncRefCount() = 0;
    virtual void DecRefCount() = 0;

    friend class IntrusiveRefCountingPtr<IWaitingState>;
};

using WaitingStatePtrT = IntrusiveRefCountingPtr<IWaitingState>;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// WaitingStateBase
///////////////////////////////////////////////////////////////////////////////////////////////////
class WaitingStateBase : public IWaitingState
{
public:
    virtual inline void Wait() override
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return !isWaiting_; });
    }

    virtual inline void IncWaitCount() override
    {
        auto oldVal = waitCount_.fetch_add(1, std::memory_order_relaxed);

        if (oldVal == 0)
        {// mutex_
            std::lock_guard<std::mutex> scopedLock(mutex_);
            isWaiting_ = true;
        }// ~mutex_
    }

    virtual inline void DecWaitCount() override
    {
        auto oldVal = waitCount_.fetch_sub(1, std::memory_order_relaxed);

        if (oldVal == 1)
        {// mutex_
            std::lock_guard<std::mutex> scopedLock(mutex_);
            isWaiting_ = false;
            condition_.notify_all();
        }// ~mutex_
    }

    inline bool IsWaiting()
    {// mutex_
        std::lock_guard<std::mutex> scopedLock(mutex_);
        return isWaiting_;
    }// ~mutex_

    template <typename F>
    auto Run(F&& func) -> decltype(func(false))
    {// mutex_
        std::lock_guard<std::mutex> scopedLock(mutex_);
        return func(isWaiting_);
    }// ~mutex_

    template <typename F>
    bool RunIfWaiting(F&& func)
    {// mutex_
        std::lock_guard<std::mutex> scopedLock(mutex_);

        if (!isWaiting_)
            return false;

        func();
        return true;            
    }// ~mutex_

    template <typename F>
    bool RunIfNotWaiting(F&& func)
    {// mutex_
        std::lock_guard<std::mutex> scopedLock(mutex_);

        if (isWaiting_)
            return false;

        func();
        return true;            
    }// ~mutex_

private:
    std::atomic<uint>           waitCount_{ 0 };
    std::condition_variable     condition_;
    std::mutex                  mutex_;
    bool                        isWaiting_ = false;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// UniqueWaitingState
///////////////////////////////////////////////////////////////////////////////////////////////////
class UniqueWaitingState : public WaitingStateBase
{
protected:
    // Do nothing
    virtual inline bool IsRefCounted() const override { return false; }
    virtual inline void IncRefCount() override {} 
    virtual inline void DecRefCount() override {}
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SharedWaitingState
///////////////////////////////////////////////////////////////////////////////////////////////////
class SharedWaitingState : public WaitingStateBase
{
private:
    SharedWaitingState() = default;

public:
    static inline WaitingStatePtrT Create()
    {
        return WaitingStatePtrT(new SharedWaitingState());
    }


protected:
    virtual inline bool IsRefCounted() const override { return true; }

    virtual inline void IncRefCount() override
    {
        refCount_.fetch_add(1, std::memory_order_relaxed);
    }

    virtual inline void DecRefCount() override
    {
        auto oldVal = refCount_.fetch_sub(1, std::memory_order_relaxed);

        if (oldVal == 1)
            delete this;
    }

private:
    std::atomic<uint>           refCount_{ 0 };
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SharedWaitingStateCollection
///////////////////////////////////////////////////////////////////////////////////////////////////
class SharedWaitingStateCollection : public IWaitingState
{
private:
    SharedWaitingStateCollection(std::vector<WaitingStatePtrT>&& others) :
        others_( std::move(others) )
    {}

public:
    static inline WaitingStatePtrT Create(std::vector<WaitingStatePtrT>&& others)
    {
        return WaitingStatePtrT(new SharedWaitingStateCollection(std::move(others)));
    }

    virtual inline bool IsRefCounted() const override { return true; }

    virtual inline void Wait() override
    {
        for (const auto& e : others_)
            e->Wait();
    }

    virtual inline void IncWaitCount() override
    {
        for (const auto& e : others_)
            e->IncWaitCount();
    }

    virtual inline void DecWaitCount() override
    {
        for (const auto& e : others_)
            e->DecWaitCount();
    }

    virtual inline void IncRefCount() override
    {
        refCount_.fetch_add(1, std::memory_order_relaxed);
    }

    virtual inline void DecRefCount() override
    {
        auto oldVal = refCount_.fetch_sub(1, std::memory_order_relaxed);

        if (oldVal == 1)
            delete this;
    }

private:
    std::atomic<uint>               refCount_{ 0 };
    std::vector<WaitingStatePtrT>   others_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// BlockingCondition
///////////////////////////////////////////////////////////////////////////////////////////////////
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