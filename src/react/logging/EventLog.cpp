
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "react/logging/EventLog.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

using std::chrono::duration_cast;
using std::chrono::microseconds;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventLog
///////////////////////////////////////////////////////////////////////////////////////////////////
EventLog::Entry::Entry() :
    time_{ std::chrono::system_clock::now() },
    data_{ nullptr }
{
}

EventLog::Entry::Entry(const Entry& other) :
    time_{ other.time_ },
    data_{ other.data_}
{
}

EventLog::Entry::Entry(IEventRecord* ptr) :
    time_{ std::chrono::system_clock::now() },
    data_{ ptr }
{
}

void EventLog::Entry::Serialize(std::ostream& out, const TimestampT& startTime) const
{
    out << EventId() << " : " << duration_cast<microseconds>(Time() - startTime).count() << std::endl;
    data_->Serialize(out);
}

EventLog::Entry& EventLog::Entry::operator=(Entry& rhs)
{
    time_ = rhs.time_,
    data_ = std::move(rhs.data_);
    return *this;
}

bool EventLog::Entry::Equals(const Entry& other) const
{
    if (EventId() != other.EventId())
        return false;
    // Todo
    return false;
}

EventLog::EventLog() :
    startTime_(std::chrono::system_clock::now())
{
}

EventLog::~EventLog()
{
    Clear();
}

void EventLog::Print()
{
    Write(std::cout);
}

void EventLog::Write(std::ostream& out)
{
    for (auto& e : entries_)
        e.Serialize(out, startTime_);
}

void EventLog::Clear()
{
    for (auto& e : entries_)
        e.Release();
    entries_.clear();
}

/****************************************/ REACT_IMPL_END /***************************************/