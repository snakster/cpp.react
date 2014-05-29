
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_COMMON_TIMING_H_INCLUDED
#define REACT_COMMON_TIMING_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <utility>

#if _WIN32 || _WIN64
    #include <windows.h>
#else
    #include <ctime>
#endif

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// GetPerformanceFrequency
///////////////////////////////////////////////////////////////////////////////////////////////////
// Todo: Initialization not thread-safe
#if _WIN32 || _WIN64
inline const LARGE_INTEGER& GetPerformanceFrequency()
{
    static bool init = false;
    static LARGE_INTEGER frequency;

    if (init == false)
    {
        QueryPerformanceFrequency(&frequency);
        init = true;
    }

    return frequency;
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ConditionalTimer
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    long long threshold,
    bool is_enabled
>
class ConditionalTimer
{
public:
    class ScopedTimer
    {
    public:
        ScopedTimer(const ConditionalTimer&);
    };

    bool IsThresholdExceeded() const;
};

// Disabled
template
<
    long long threshold
>
class ConditionalTimer<threshold,false>
{
public:
    // Defines scoped timer that does nothing
    class ScopedTimer
    {
    public:
        ScopedTimer(const ConditionalTimer&) {}
    };

    bool IsThresholdExceeded() const { return false; }
};

// Enabled
template
<
    long long threshold
>
class ConditionalTimer<threshold,true>
{
public:
#if _WIN32 || _WIN64
    using TimestampT = LARGE_INTEGER;
#elif __linux__
    using TimestampT = long long;
#endif

    class ScopedTimer
    {
    public:
        ScopedTimer(ConditionalTimer& parent) :
            parent_( parent )
        {
            if (!parent_.shouldMeasure_)
                return;

            startMeasure();
        }

        ~ScopedTimer()
        {
            if (!parent_.shouldMeasure_)
                return;

            parent_.shouldMeasure_ = false;
            
            endMeasure();
        }

    private:
        void startMeasure()
        {
            startTime_ = now();
        }

        void endMeasure()
        {
            TimestampT endTime = now();

#if _WIN32 || _WIN64
            LARGE_INTEGER durationMS;

            durationMS.QuadPart = endTime.QuadPart - startTime_.QuadPart;
            durationMS.QuadPart *= 1000000;
            durationMS.QuadPart /= GetPerformanceFrequency().QuadPart;

            parent_.isThresholdExceeded_ = durationMS.QuadPart > threshold;
#else
            // TODO
            parent_.isThresholdExceeded_ = false;
#endif
        }

        ConditionalTimer&   parent_;
        TimestampT          startTime_;  
    };

    static TimestampT now()
    {
#if _WIN32 || _WIN64
        TimestampT result;
        QueryPerformanceCounter(&result);
        return result;
#else
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        return static_cast<long long>(1000000000UL)
            * static_cast<long long>(ts.tv_sec)
            + static_cast<long long>(ts.tv_nsec);
#endif
    }

    bool IsThresholdExceeded() const { return isThresholdExceeded_; }

private:
    // Only measure once
    bool shouldMeasure_ = true;
    bool isThresholdExceeded_ = false;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_COMMON_TIMING_H_INCLUDED