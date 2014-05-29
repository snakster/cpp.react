
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_COMMON_CONTAINERS_H_INCLUDED
#define REACT_COMMON_CONTAINERS_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <algorithm>
#include <array>
#include <type_traits>
#include <vector>

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EnumFlags
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
class EnumFlags
{
public:
    using FlagsT = typename std::underlying_type<T>::type;

    template <T x>
    void Set() { flags_ |= 1 << x; }

    template <T x>
    void Clear() { flags_ &= ~(1 << x); }

    template <T x>
    bool Test() const { return (flags_ & (1 << x)) != 0; }

private:
    FlagsT  flags_ = 0;
};

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
/// NodeBuffer
///////////////////////////////////////////////////////////////////////////////////////////////////
struct SplitTag {};

template <typename T, size_t N>
class NodeBuffer
{
public:
    using DataT = std::array<T*,N>;
    using iterator = typename DataT::iterator;
    using const_iterator = typename DataT::const_iterator;

    static const size_t split_size = N / 2;

    NodeBuffer() :
        size_( 0 ),
        front_( nodes_.begin() ),
        back_( nodes_.begin() )
    {}

    NodeBuffer(T* node) :
        size_( 1 ),
        front_( nodes_.begin() ),
        back_( nodes_.begin() + 1 )
    {
        nodes_[0] = node;
    }

    template <typename TInput>
    NodeBuffer(TInput srcBegin, TInput srcEnd) :
        size_( std::distance(srcBegin, srcEnd) ), // parentheses to allow narrowing conversion
        front_( nodes_.begin() ),
        back_( size_ != N ? nodes_.begin() + size_ : nodes_.begin() )
    {
        std::copy(srcBegin, srcEnd, front_);
    }

    // Other must be full
    NodeBuffer(NodeBuffer& other, SplitTag) :
        size_( split_size ),
        front_( nodes_.begin() ),
        back_( nodes_.begin() )
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
    inline void increment(iterator& it)
    {
        if (++it == nodes_.end())
            it = nodes_.begin();
    }

    inline void decrement(iterator& it)
    {
        if (it == nodes_.begin())
            it = nodes_.end();
        --it;
    }

    DataT       nodes_;
    size_t      size_;
    iterator    front_;
    iterator    back_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_COMMON_CONTAINERS_H_INCLUDED