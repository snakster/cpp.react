
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
    #define REACT_FIXME_CUSTOM_TIMER 1
#else
    #define REACT_FIXME_CUSTOM_TIMER 0
#endif

#if REACT_FIXME_CUSTOM_TIMER
    #include <windows.h>
#else
    #include <chrono>
#endif

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// GetPerformanceFrequency
///////////////////////////////////////////////////////////////////////////////////////////////////
// Todo: Initialization not thread-safe
#if REACT_FIXME_CUSTOM_TIMER
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
#if REACT_FIXME_CUSTOM_TIMER
    using TimestampT = LARGE_INTEGER;
#else
    using ClockT = std::chrono::high_resolution_clock;
    using TimestampT = std::chrono::time_point<ClockT>;
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
#if REACT_FIXME_CUSTOM_TIMER
            TimestampT endTime = now();

            LARGE_INTEGER durationUS;

            durationUS.QuadPart = endTime.QuadPart - startTime_.QuadPart;
            durationUS.QuadPart *= 1000000;
            durationUS.QuadPart /= GetPerformanceFrequency().QuadPart;

            parent_.isThresholdExceeded_ = durationUS.QuadPart > threshold;
#else
            using std::chrono::duration_cast;
            using std::chrono::microseconds;

            parent_.isThresholdExceeded_ =
                duration_cast<microseconds>(now() - startTime_).count() > threshold;      
#endif
        }

        ConditionalTimer&   parent_;
        TimestampT          startTime_;  
    };

    static TimestampT now()
    {
#if REACT_FIXME_CUSTOM_TIMER
        TimestampT result;
        QueryPerformanceCounter(&result);
        return result;
#else
        return ClockT::now();
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