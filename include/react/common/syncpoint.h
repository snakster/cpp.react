
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_COMMON_SYNCPOINT_H_INCLUDED
#define REACT_COMMON_SYNCPOINT_H_INCLUDED

#pragma once

#include "react/detail/defs.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <iterator>
#include <mutex>


/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncPoint
///////////////////////////////////////////////////////////////////////////////////////////////////
class SyncPoint
{
public:
    class Dependency;

private:
    

    class ISyncTarget
    {
    public:
        virtual ~ISyncTarget() = default;

        virtual void IncrementWaitCount() = 0;
        
        virtual void DecrementWaitCount() = 0;
    };

    class SyncPointState : public ISyncTarget
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

    class SyncTargetCollection : public ISyncTarget
    {
    public:
        virtual void IncrementWaitCount() override
        {
            for (const auto& e : targets)
                e->IncrementWaitCount();
        }

        virtual void DecrementWaitCount() override
        {
            for (const auto& e : targets)
                e->DecrementWaitCount();
        }

        std::vector<std::shared_ptr<ISyncTarget>> targets;
    };

public:
    SyncPoint() :
        state_( std::make_shared<SyncPointState>() )
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
            target_( sp.state_ )
        {
            target_->IncrementWaitCount();
        }

        // Construct from vector of other dependencies.
        template <typename TBegin, typename TEnd>
        Dependency(TBegin first, TEnd last)
        {
            auto count = std::distance(first, last);

            if (count == 1)
            {
                target_ = first->target_;
                if (target_)
                    target_->IncrementWaitCount();
            }
            else if (count > 1)
            {
                auto collection = std::make_shared<SyncTargetCollection>();
                collection->targets.reserve(count);
            
                for (; !(first == last); ++first)
                    if (first->target_) // There's no point in propagating released/empty dependencies.
                        collection->targets.push_back(first->target_);

                collection->IncrementWaitCount();

                target_ = std::move(collection);
            }
        }

        Dependency(const Dependency& other) :
            target_( other.target_ )
        {
            if (target_)
                target_->IncrementWaitCount();
        }

        Dependency& operator=(const Dependency& other)
        {
            if (other.target_)
                other.target_->IncrementWaitCount();

            if (target_)
                target_->DecrementWaitCount();

            target_ = other.target_;
            return *this;
        }

        Dependency(Dependency&& other) :
            target_( std::move(other.target_) )
        { }

        Dependency& operator=(Dependency&& other)
        {
            if (target_)
                target_->DecrementWaitCount();

            target_ = std::move(other.target_);
            return *this;
        }

        ~Dependency()
        {
            if (target_)
                target_->DecrementWaitCount();
        }

        void Release()
        {
            if (target_)
            {
                target_->DecrementWaitCount();
                target_ = nullptr;
            }
        }

        bool IsReleased() const
            { return target_ == nullptr; }

    private:
        std::shared_ptr<ISyncTarget> target_;
    };

private:
    std::shared_ptr<SyncPointState> state_;
};

/******************************************/ REACT_END /******************************************/

#endif // REACT_COMMON_SYNCPOINT_H_INCLUDED