
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/Defs.h"

#include <algorithm>
#include <array>
#include <vector>

#include "tbb/enumerable_thread_specific.h"
#include "tbb/tbb_stddef.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

template <typename T>
struct NodeLevelHelper
{
    int operator()(const T& x) const { return x.Level; }
};

template <typename T>
struct NodeLevelHelper<T*>
{
    int operator()(const T* x) const { return x->Level; }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Sequential TopoQueue
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
class TopoQueue
{
public:
    using DataT = std::vector<T>;
    using LevelFunctorT = NodeLevelHelper<T>;

    void Push(const T& node)
    {
        data_.push_back(node);
    }

    bool FetchNext()
    {
        next_.clear();

        minLevel_ = INT_MAX;
        for (const auto& e : data_)
        {
            auto l = LevelFunctorT{}(e);
            if (minLevel_ > l)
                minLevel_ = l;
        }

        auto p = std::partition(data_.begin(), data_.end(), CompFunctor{ minLevel_ });

        next_.insert(next_.end(), p, data_.end());
        data_.resize(std::distance(data_.begin(), p));

        return !next_.empty();
    }

    const DataT& NextNodes() const  { return next_; }

private:
    struct CompFunctor
    {
        CompFunctor(int level) : Level{ level } {}
        bool operator()(const T& x) { return LevelFunctorT{}(x) != Level; }
        const int Level;
    };

    DataT   next_;
    DataT   data_;
    int     minLevel_ = INT_MAX;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// WeightedRange
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct NodeWeightHelper
{
    int operator()(const T& x) const { return x.Weight; }
};

template <typename T>
struct NodeWeightHelper<T*>
{
    int operator()(const T* x) const { return x->Weight; }
};

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
    using WeightFunctorT = NodeWeightHelper<ValueT>;

    WeightedRange() = default;
    WeightedRange(const WeightedRange& other) = default;

    WeightedRange(const TIt& a, const TIt& b, uint weight) :
        begin_{ a },
        end_{ b },
        weight_{ weight }
    {
    }

    WeightedRange(WeightedRange& source, tbb::split)
    {
        uint sum = 0;        
        TIt p = source.begin_;
        while (p != source.end_)
        {
            sum += WeightFunctorT{}(*p);
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
    TIt     begin_;
    TIt     end_;
    uint    weight_ = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ConcurrentTopoQueue
/// Usage based on two phases:
///     1. Multiple threads push nodes to the queue concurrently.
///     2. FetchNext() prepares all nodes of the next level in NextNodes().
///         The previous contents of NextNodes() are automatically cleared. 
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T, uint grain_size>
class ConcurrentTopoQueue
{
public:
    using DataT = std::vector<T>;
    using RangeT = WeightedRange<typename DataT::const_iterator, grain_size>;
    using LevelFunctorT = NodeLevelHelper<T>;
    using WeightFunctorT = NodeWeightHelper<T>;

    void Push(const T& node)
    {
        auto& t = collectBuffer_.local();
        
        t.Data.push_back(node);
        t.Weight += WeightFunctorT{}(node);
        auto l = LevelFunctorT{}(node);
        if (t.MinLevel > l)
            t.MinLevel = l;
    }

    bool FetchNext()
    {
        nodes_.clear();
        uint totalWeight = 0;

        // Determine current min level
        minLevel_ = INT_MAX;
        for (const auto& buf : collectBuffer_)
            if (minLevel_ > buf.MinLevel)
                minLevel_ = buf.MinLevel;

        // For each thread local buffer...
        for (auto& buf : collectBuffer_)
        {
            auto& v = buf.Data;

            // Swap min level nodes to end of v
            auto p = std::partition(v.begin(), v.end(), CompFunctor{ minLevel_ });

            // Copy them to global nodes_
            std::copy(p, v.end(), std::back_inserter(nodes_));

            // Truncate remaining
            v.resize(std::distance(v.begin(), p));

            // Calc new min level and weight for this buffer
            buf.MinLevel = INT_MAX;
            int oldWeight = buf.Weight;
            buf.Weight = 0;
            for (const T& x : v)
            {
                buf.Weight += WeightFunctorT{}(x);
                auto l = LevelFunctorT{}(x);
                if (buf.MinLevel > l)
                    buf.MinLevel = l;
            }

            // Add diff to nodes_ weight
            totalWeight += oldWeight - buf.Weight;
        }

        range_ = RangeT(nodes_.begin(), nodes_.end(), totalWeight);

        // Found more nodes?
        return !nodes_.empty();
    }

    const RangeT& NextRange() const
    {
        return range_;
    }

private:
    struct CompFunctor
    {
        CompFunctor(int level) : Level{ level } {}
        bool operator()(const T& x) { return  LevelFunctorT{}(x) != Level; }
        const int Level;
    };

    struct ThreadLocalBuffer
    {
        DataT   Data;
        int     MinLevel = INT_MAX;
        uint    Weight = 0;
    };

    int                 minLevel_ = INT_MAX;
    DataT               nodes_;
    RangeT              range_;

    tbb::enumerable_thread_specific<ThreadLocalBuffer>    collectBuffer_;
};

/****************************************/ REACT_IMPL_END /***************************************/