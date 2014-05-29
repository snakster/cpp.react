
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_COMMON_TOPOQUEUE_H_INCLUDED
#define REACT_COMMON_TOPOQUEUE_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <algorithm>
#include <array>
#include <limits>
#include <utility>
#include <vector>

#include "tbb/enumerable_thread_specific.h"
#include "tbb/tbb_stddef.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// TopoQueue - Sequential
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T, typename TLevelFunc>
class TopoQueue
{
private:
    struct Entry;

public:
    // Store the level as part of the entry for cheap comparisons
    using QueueDataT    = std::vector<Entry>;
    using NextDataT     = std::vector<T>;

    TopoQueue() = default;
    TopoQueue(const TopoQueue&) = default;

    template <typename FIn>
    TopoQueue(FIn&& levelFunc) :
        levelFunc_( std::forward<FIn>(levelFunc) )
    {}

    void Push(const T& value)
    {
        queueData_.emplace_back(value, levelFunc_(value));
    }

    bool FetchNext()
    {
        // Throw away previous values
        nextData_.clear();

        // Find min level of nodes in queue data
        minLevel_ = (std::numeric_limits<int>::max)();
        for (const auto& e : queueData_)
            if (minLevel_ > e.Level)
                minLevel_ = e.Level;

        // Swap entries with min level to the end
        auto p = std::partition(
            queueData_.begin(),
            queueData_.end(),
            LevelCompFunctor{ minLevel_ });

        // Reserve once to avoid multiple re-allocations
        nextData_.reserve(std::distance(p, queueData_.end()));

        // Move min level values to next data
        for (auto it = p; it != queueData_.end(); ++it)
            nextData_.push_back(std::move(it->Value));

        // Truncate moved entries
        queueData_.resize(std::distance(queueData_.begin(), p));

        return !nextData_.empty();
    }

    const NextDataT& NextValues() const  { return nextData_; }

private:
    struct Entry
    {
        Entry() = default;
        Entry(const Entry&) = default;

        Entry(const T& value, int level) : Value( value ), Level( level ) {}

        T       Value;
        int     Level;
    };

    struct LevelCompFunctor
    {
        LevelCompFunctor(int level) : Level( level ) {}

        bool operator()(const Entry& e) const { return e.Level != Level; }

        const int Level;
    };

    NextDataT   nextData_;
    QueueDataT  queueData_;

    TLevelFunc  levelFunc_;

    int         minLevel_ = (std::numeric_limits<int>::max)();
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// WeightedRange - Implements tbb range concept
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename TIt,
    uint grain_size
>
class WeightedRange
{
public:
    using const_iterator = TIt;
    using ValueT = typename TIt::value_type;

    WeightedRange() = default;
    WeightedRange(const WeightedRange&) = default;

    WeightedRange(const TIt& a, const TIt& b, uint weight) :
        begin_( a ),
        end_( b ),
        weight_( weight )
    {}

    WeightedRange(WeightedRange& source, tbb::split)
    {
        uint sum = 0;        
        TIt p = source.begin_;
        while (p != source.end_)
        {
            // Note: assuming a pair with weight as second until more flexibility is needed
            sum += p->second;
            ++p;
            if (sum >= grain_size)
                break;
        }

        // New [p,b)
        begin_ = p;
        end_ = source.end_;
        weight_ =  source.weight_ - sum;

        // Source [a,p)
        source.end_ = p;
        source.weight_ = sum;
    }

    // tbb range interface
    bool empty() const          { return !(Size() > 0); }
    bool is_divisible() const   { return  weight_ > grain_size && Size() > 1; }

    // iteration interface
    const_iterator begin() const    { return begin_; }
    const_iterator end() const      { return end_; }

    size_t  Size() const    { return end_ - begin_; }
    uint    Weight() const  { return weight_; }

private:
    TIt         begin_;
    TIt         end_;
    uint        weight_ = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ConcurrentTopoQueue
/// Usage based on two phases:
///     1. Multiple threads push nodes to the queue concurrently.
///     2. FetchNext() prepares all nodes of the next level in NextNodes().
///         The previous contents of NextNodes() are automatically cleared. 
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<   typename T,
    uint grain_size,
    typename TLevelFunc,
    typename TWeightFunc
>
class ConcurrentTopoQueue
{
private:
    struct Entry;

public:
    using QueueDataT    = std::vector<Entry>;
    using NextDataT     = std::vector<std::pair<T,uint>>;
    using NextRangeT    = WeightedRange<typename NextDataT::const_iterator, grain_size>;

    ConcurrentTopoQueue() = default;
    ConcurrentTopoQueue(const ConcurrentTopoQueue&) = default;

    template <typename FIn1, typename FIn2>
    ConcurrentTopoQueue(FIn1&& levelFunc, FIn2&& weightFunc) :
        levelFunc_( std::forward<FIn1>(levelFunc) ),
        weightFunc_( std::forward<FIn2>(weightFunc) )
    {}

    void Push(const T& value)
    {
        auto& t = collectBuffer_.local();

        auto level  = levelFunc_(value);
        auto weight = weightFunc_(value);

        t.Data.emplace_back(value,level,weight);

        t.Weight += weight;

        if (t.MinLevel > level)
            t.MinLevel = level;
    }

    bool FetchNext()
    {
        nextData_.clear();
        uint totalWeight = 0;

        // Determine current min level
        minLevel_ = (std::numeric_limits<int>::max)();
        for (const auto& buf : collectBuffer_)
            if (minLevel_ > buf.MinLevel)
                minLevel_ = buf.MinLevel;

        // For each thread local buffer...
        for (auto& buf : collectBuffer_)
        {
            auto& v = buf.Data;

            // Swap min level nodes to end of v
            auto p = std::partition(
                v.begin(),
                v.end(),
                LevelCompFunctor{ minLevel_ });

            // Reserve once to avoid multiple re-allocations
            nextData_.reserve(std::distance(p, v.end()));

            // Move min level values to global next data
            for (auto it = p; it != v.end(); ++it)
                nextData_.emplace_back(std::move(it->Value), it->Weight);

            // Truncate remaining
            v.resize(std::distance(v.begin(), p));

            // Calc new min level and weight for this buffer
            buf.MinLevel = (std::numeric_limits<int>::max)();
            int oldWeight = buf.Weight;
            buf.Weight = 0;
            for (const auto& x : v)
            {
                buf.Weight += x.Weight;

                if (buf.MinLevel > x.Level)
                    buf.MinLevel = x.Level;
            }

            // Add diff to nodes_ weight
            totalWeight += oldWeight - buf.Weight;
        }

        nextRange_ = NextRangeT{ nextData_.begin(), nextData_.end(), totalWeight };

        // Found more nodes?
        return !nextData_.empty();
    }

    const NextRangeT& NextRange() const
    {
        return nextRange_;
    }

private:
    struct Entry
    {
        Entry() = default;
        Entry(const Entry&) = default;

        Entry(const T& value, int level, uint weight) :
            Value( value ),
            Level( level ),
            Weight( weight )
        {}

        T       Value;
        int     Level;
        uint    Weight;
    };

    struct LevelCompFunctor
    {
        LevelCompFunctor(int level) : Level{ level } {}

        bool operator()(const Entry& e) const { return  e.Level != Level; }

        const int Level;
    };

    struct ThreadLocalBuffer
    {
        QueueDataT  Data;
        int         MinLevel = (std::numeric_limits<int>::max)();
        uint        Weight = 0;
    };

    int         minLevel_ = (std::numeric_limits<int>::max)();

    NextDataT   nextData_;
    NextRangeT  nextRange_;

    TLevelFunc  levelFunc_;
    TWeightFunc weightFunc_;

    tbb::enumerable_thread_specific<ThreadLocalBuffer>    collectBuffer_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_COMMON_TOPOQUEUE_H_INCLUDED