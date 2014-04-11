
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/Defs.h"

#include <algorithm>
#include <array>
#include <vector>

#include <boost/circular_buffer.hpp>

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// NodeVector
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TNode>
class NodeVector
{
private:
    typedef std::vector<TNode*>    DataT;

public:
    void Add(TNode& node)
    {
        data_.push_back(&node);
    }

    void Remove(const TNode& node)
    {
        data_.erase(std::find(data_.begin(), data_.end(), &node));
    }

    typedef typename DataT::iterator        iterator;
    typedef typename DataT::const_iterator  const_iterator;

    iterator    begin() { return data_.begin(); }
    iterator    end()   { return data_.end(); }

    const_iterator begin() const    { return data_.begin(); }
    const_iterator end() const      { return data_.end(); }

private:
    DataT    data_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// NodeStack
///////////////////////////////////////////////////////////////////////////////////////////////////
struct SplitTag {};

template <typename T, int N>
class NodeStack
{
public:
    using DataT = std::array<T*,N>;
    using iterator = typename DataT::iterator;
    using const_iterator = typename DataT::const_iterator;

    static const int split_size = N / 2;

    NodeStack() :
        cursor_{ nodes_.begin() }
    {}

    template <typename TInput>
    NodeStack(TInput srcBegin, TInput srcEnd) :
        cursor_{ nodes_.begin() + std::distance(srcBegin, srcEnd) }
    {
        std::copy(srcBegin, srcEnd, nodes_.begin());
    }

    // Other must be full
    NodeStack(NodeStack& other, SplitTag) :
        cursor_{ nodes_.begin() + split_size }
    {
        std::copy(other.nodes_.begin() + split_size, other.nodes_.end(), nodes_.begin());
        other.cursor_ = other.nodes_.begin() + split_size;
    }

    void Push(T* e)         { *cursor_ = e; ++cursor_; }
    T*   Pop()              { --cursor_; return *cursor_; }

    bool IsFull() const     { return cursor_ == nodes_.end(); }
    bool IsEmpty() const    { return cursor_ == nodes_.begin(); }

    iterator        begin()         { return nodes_.begin(); }
    const_iterator  begin() const   { return nodes_.begin(); }
    iterator        end()           { return cursor_; }
    const_iterator  end() const     { return cursor_; }

private:
    DataT       nodes_;
    iterator    cursor_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// NodeBuffer
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T, int N>
class NodeBuffer
{
public:
    using DataT = std::array<T*,N>;
    using iterator = typename DataT::iterator;
    using const_iterator = typename DataT::const_iterator;

    static const int split_size = N / 2;

    NodeBuffer() :
        size_{ 0 },
        front_{ nodes_.begin() },
        back_{ nodes_.begin() }
    {}

    template <typename TInput>
    NodeBuffer(TInput srcBegin, TInput srcEnd) :
        size_{ std::distance(srcBegin, srcEnd) },
        front_{ nodes_.begin() },
        back_{ nodes_.begin() + size_}
    {
        std::copy(srcBegin, srcEnd, front_);
    }

    // Other must be full
    NodeBuffer(NodeBuffer& other, SplitTag) :
        size_{ split_size },
        front_{ nodes_.begin() },
        back_{ nodes_.begin() }
    {
        for (auto i=0; i<split_size; i++)
            *(back_++) = other.PopFront();
    }

    void PushFront(T* e)
    {
        size_++;
        decrement(front_);
        *front_ = e;
    }

    void PushBack(T* e)
    {
        size_++;
        *back_ = e;
        increment(back_);
    }

    T* PopFront()
    {
        size_--;
        auto t = *front_;
        increment(front_);
        return t;
    }

    T* PopBack()
    {
        size_--;
        decrement(back_);
        return *back_;
    }

    bool IsFull() const     { return size_ == N; }
    bool IsEmpty() const    { return size_ == 0; }

private:
    //! Increment the pointer.
    inline void increment(iterator& it)
    {
        if (++it == nodes_.end())
            it = nodes_.begin();
    }

    //! Decrement the pointer.
    inline void decrement(iterator& it)
    {
        if (it == nodes_.begin())
            it = nodes_.end();
        --it;
    }

    DataT       nodes_;
    int         size_;
    iterator    front_;
    iterator    back_;
};

/****************************************/ REACT_IMPL_END /***************************************/