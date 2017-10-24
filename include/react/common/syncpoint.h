
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_COMMON_SYNCPOINT_H_INCLUDED
#define REACT_COMMON_SYNCPOINT_H_INCLUDED

#pragma once

#include "react/detail/defs.h"

#include <chrono>
#include <condition_variable>
#include <mutex>


/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncPoint
///////////////////////////////////////////////////////////////////////////////////////////////////
class SyncPoint
{
private:
    class Dependency;

    class ISyncPointState
    {
    public:
        virtual ~ISyncPointState() = default;

        virtual void IncrementWaitCount() = 0;
        
        virtual void DecrementWaitCount() = 0;
    };

    class SyncPointState : public ISyncPointState
    {
    public:
        virtual void IncrementWaitCount() override
        {// mutex_
            std::lock_guard<std::mutex> scopedLock(mtx_);
            ++waitCount_;
        }// ~mutex_

        virtual void DecrementWaitCount() override
        {// mutex_
            std::lock_guard<std::mutex> scopedLock(mtx_);
            --waitCount_;

            if (waitCount_ == 0)
                cv_.notify_all();
        }// ~mutex_

        void Wait()
        {
            std::unique_lock<std::mutex> lock(mtx_);
            cv_.wait(lock, [this] { return waitCount_ == 0; });
        }
        
        template <typename TRep, typename TPeriod>
        bool WaitFor(const std::chrono::duration<TRep, TPeriod>& relTime)
        {
            std::unique_lock<std::mutex> lock(mtx_);
            return cv_.wait_for(lock, relTime, [this] { return waitCount_ == 0; });
        }

        template <typename TRep, typename TPeriod>
        bool WaitUntil(const std::chrono::duration<TRep, TPeriod>& relTime)
        {
            std::unique_lock<std::mutex> lock(mtx_);
            return cv_.wait_until(lock, relTime, [this] { return waitCount_ == 0; });
        }

    private:
        std::mutex              mtx_;
        std::condition_variable cv_;

        int waitCount_ = 0;
    };

    class SyncPointStateCollection : public ISyncPointState
    {
    private:
        std::vector<Dependency>
    };

public:
    SyncPoint() : state_( std::make_shared<SyncPointState>() )
    { }

    SyncPoint(const SyncPoint&) = default;

    SyncPoint& operator=(const SyncPoint&) = default;

    SyncPoint(SyncPoint&&) = default;

    SyncPoint& operator=(SyncPoint&&) = default;

    void Wait()
    {
        state_->Wait();
    }

    template <typename TRep, typename TPeriod>
    bool WaitFor(const std::chrono::duration<TRep, TPeriod>& relTime)
    {
        return state_->WaitFor(relTime);
    }

    template <typename TRep, typename TPeriod>
    bool WaitUntil(const std::chrono::duration<TRep, TPeriod>& relTime)
    {
        return state_->WaitUntil(relTime);
    }

    class Dependency
    {
    public:
        Dependency() = default;

        // Construct from single sync point.
        explicit Dependency(const SyncPoint& sp) :
            state_( sp.state_ )
        {
            state_->IncrementWaitCount();
        }

        // Construct from vector of other dependencies.
        explicit Dependency(std::vector<Dependency>&& others) :
            state_( sp.state_ )
        {
            state_->IncrementWaitCount();
        }

        Dependency(const Dependency& other) :
            state_( other.state_ )
        {
            state_->IncrementWaitCount();
        }

        Dependency& operator=(const Dependency& other)
        {
            if (other.state_)
                state_->IncrementWaitCount();

            if (state_)
                state_->DecrementWaitCount();

            state_ = other.state_;
        }

        Dependency(Dependency&& other) :
            state_( std::move(other.state_) )
        { }

        Dependency& operator=(Dependency&& other)
        {
            if (state_)
                state_->DecrementWaitCount();

            state_ = std::move(other.state_);
        }

        ~Dependency()
        {
            if (state_)
                state_->DecrementWaitCount();
        }

        void Release()
        {
            if (state_)
            {
                state_->DecrementWaitCount();
                state_.reset();
            }
        }

        bool IsReleased() const
            { return state_ == nullptr; }

    private:
        std::shared_ptr<ISyncPointState> state_;
    };

private:
    std::shared_ptr<SyncPointState> state_;
};

/******************************************/ REACT_END /******************************************/

#endif // REACT_COMMON_SYNCPOINT_H_INCLUDED