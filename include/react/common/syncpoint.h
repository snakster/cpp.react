
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
#include <vector>


/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// A synchronization primitive that is essentially a cooperative semaphore, i.e. it blocks a
/// consumer until all producers are done.
/// Usage:
///     SyncPoint sp;
///     // Add a dependency the sync point should wait for. The depenency is released on destruction.
///     SyncPoint::Depenency dep(sp);
///     // Pass dependency to some operation. Dependencies can be copied as well.
///     SomeAsyncOperation(std::move(dep));
///     // Wait until all dependencies are released.
///     sp.Wait()
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
    /// Creates a sync point.
    SyncPoint() :
        state_( std::make_shared<SyncPointState>() )
    { }

    SyncPoint(const SyncPoint&) = default;

    SyncPoint& operator=(const SyncPoint&) = default;

    SyncPoint(SyncPoint&&) = default;

    SyncPoint& operator=(SyncPoint&&) = default;

    /// Blocks the calling thread until all dependencies of this sync point are released.
    void Wait()
    {
        state_->Wait();
    }

    /// Like Wait, but times out after relTime. Returns false if the timeout was hit.
    template <typename TRep, typename TPeriod>
    bool WaitFor(const std::chrono::duration<TRep, TPeriod>& relTime)
    {
        return state_->WaitFor(relTime);
    }

    /// Like Wait, but times out at relTime. Returns false if the timeout was hit.
    template <typename TRep, typename TPeriod>
    bool WaitUntil(const std::chrono::duration<TRep, TPeriod>& relTime)
    {
        return state_->WaitUntil(relTime);
    }

    /// A RAII-style token object that represents a dependency of a SyncPoint.
    class Dependency
    {
    public:
        /// Constructs an unbound dependency.
        Dependency() = default;

        /// Constructs a single dependency for a sync point.
        explicit Dependency(const SyncPoint& sp) :
            target_( sp.state_ )
        {
            target_->IncrementWaitCount();
        }

        /// Merges an input range of other dependencies into a single dependency.
        /// This allows to create APIs that are agnostic of how many dependent operations they process.        
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

        /// Copy constructor and assignment split a dependency.
        /// The new dependency that has the same target(s) as other.
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

        /// Move constructor and assignment transfer a dependency.
        /// The moved from object is left unbound.
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

        /// The destructor releases a dependency, if it's not unbound.
        ~Dependency()
        {
            if (target_)
                target_->DecrementWaitCount();
        }

        /// Manually releases the dependency. Afterwards it is unbound.
        void Release()
        {
            if (target_)
            {
                target_->DecrementWaitCount();
                target_ = nullptr;
            }
        }

        /// Returns if a dependency is released, i.e. if it is unbound.
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