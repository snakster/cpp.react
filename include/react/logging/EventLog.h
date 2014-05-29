
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_LOGGING_EVENTLOG_H_INCLUDED
#define REACT_DETAIL_LOGGING_EVENTLOG_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <memory>
#include <cstdint>
#include <functional>
#include <iostream>
#include <chrono>

#include "tbb/concurrent_vector.h"

#include "Logging.h"
#include "react/common/Types.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventLog
///////////////////////////////////////////////////////////////////////////////////////////////////
class EventLog
{
    using TimestampT = std::chrono::time_point<std::chrono::high_resolution_clock>;

    class Entry
    {
    public:
        Entry();
        Entry(const Entry& other);

        explicit Entry(IEventRecord* ptr);

        Entry& operator=(Entry& rhs);

        inline const char* EventId() const
        {
            return data_->EventId();
        }

        inline const TimestampT& Time() const
        {
            return time_;
        }

        inline bool operator<(const Entry& other) const
        {
            return time_ < other.time_;
        }

        inline void Release()
        {
            delete data_;
        }

        void Serialize(std::ostream& out, const TimestampT& startTime) const;
        bool Equals(const Entry& other) const;

    private:
        TimestampT      time_;
        IEventRecord*   data_;
    };

public:

    EventLog();
    ~EventLog();

    void    Print();
    void    Write(std::ostream& out);
    void    Clear();

    template
    <
        typename TEventRecord,
        typename ... TArgs
    >
    void Append(TArgs ... args)
    {
        entries_.push_back(Entry(new TEventRecord(args ...)));
    }

private:
    using LogEntriesT = tbb::concurrent_vector<Entry>;

    LogEntriesT     entries_;
    TimestampT      startTime_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_LOGGING_EVENTLOG_H_INCLUDED