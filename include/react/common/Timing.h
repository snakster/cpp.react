
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/detail/Defs.h"

#include <utility>

// Todo: Make portable
#include <windows.h>

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// GetPerformanceFrequency
///////////////////////////////////////////////////////////////////////////////////////////////////
// Todo: Initialization not thread-safe
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

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ConditionalTimer
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    long long heavy_threshold,
    bool is_enabled
>
class ConditionalTimer;

// Disabled
template
<
    long long threshold
>
class ConditionalTimer<threshold,false>
{
public:
    // Defines scoped timer that does nothing
    struct ScopedTimer
    {
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
    class ScopedTimer
    {
    public:
        ScopedTimer(ConditionalTimer& parent) :
            parent_{ parent }
        {
            if (parent_.shouldMeasure_)
                QueryPerformanceCounter(&startTime_);
        }

        ~ScopedTimer()
        {
            if (!parent_.shouldMeasure_)
                return;

            parent_.shouldMeasure_ = false;

            LARGE_INTEGER endTime, durationMS;

            QueryPerformanceCounter(&endTime);

            durationMS.QuadPart = endTime.QuadPart - startTime_.QuadPart;
            durationMS.QuadPart *= 1000000;
            durationMS.QuadPart /= GetPerformanceFrequency().QuadPart;

            parent_.isThresholdExceeded_ = durationMS.QuadPart > threshold;
        }

    private:
        ConditionalTimer& parent_;
        
        LARGE_INTEGER startTime_;  
    };

    bool IsThresholdExceeded() const { return isThresholdExceeded_; }

private:
    // Only measure once
    bool shouldMeasure_ = true;
    bool isThresholdExceeded_ = false;
};

/****************************************/ REACT_IMPL_END /***************************************/
