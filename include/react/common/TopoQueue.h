
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

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// TopoQueue
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
class TopoQueue
{
public:
    static bool LevelOrderOp(const T* lhs, const T* rhs)
    {
        return lhs->Level > rhs->Level;
    }

    void Push(T* node)
    {
        data_.push_back(node);
        std::push_heap(data_.begin(), data_.end(), LevelOrderOp);
    }

    void Pop()
    {
        std::pop_heap(data_.begin(), data_.end(), LevelOrderOp);
        data_.pop_back();
    }

    T* Top() const
    {
        return data_.front();
    }

    bool Empty() const
    {
        return data_.empty();
    }

    void Invalidate()
    {
        std::make_heap(data_.begin(), data_.end(), LevelOrderOp);
    }

private:
    std::vector<T*>    data_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ConcurrentTopoQueue
/// Usage based on two phases:
///     1. Multiple threads push nodes to the queue concurrently.
///     2. FetchNodes() prepares all nodes of the next level in NextNodes().
///         The previous contents of NextNodes() are automatically cleared. 
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
class ConcurrentTopoQueue
{
public:
    void Push(T* node)
    {
        auto& t = collectBuffer_.local();
        
        t.Data.push_back(node);
        if (t.MinLevel > node->Level)
            t.MinLevel = node->Level;
    }

    bool FetchNextNodes()
    {
        nextNodes_.clear();

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
            auto p = std::partition(v.begin(), v.end(), Comp_{ minLevel_ });

            // Copy them to global nextNodes_
            std::copy(p, v.end(), std::back_inserter(nextNodes_));

            // Truncate remaining
            v.resize(std::distance(v.begin(), p));

            // Calc new min level for this buffer
            buf.MinLevel = INT_MAX;
            for (const T* x : v)
                if (buf.MinLevel > x->Level)
                    buf.MinLevel = x->Level;
        }

        // Found more nodes?
        return !nextNodes_.empty();
    }

    const std::vector<T*>& NextNodes() const
    {
        return nextNodes_;
    }

private:
    struct Comp_
    {
        const int Level;
        Comp_(int level) : Level{ level } {}
        bool operator()(T* other) { return other->Level != Level; }
    };

    struct ThreadLocalBuffer_
    {
        int             MinLevel = INT_MAX;
        std::vector<T*> Data;
    };

    int                 minLevel_ = INT_MAX;
    std::vector<T*>     nextNodes_;

    tbb::enumerable_thread_specific<ThreadLocalBuffer_>    collectBuffer_;
};

/****************************************/ REACT_IMPL_END /***************************************/