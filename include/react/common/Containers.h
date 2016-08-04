
//          Copyright Sebastian Jeckel 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_COMMON_CONTAINERS_H_INCLUDED
#define REACT_COMMON_CONTAINERS_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <algorithm>
#include <array>
#include <iterator>
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
/// IndexMap
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
class IndexMap
{
    static const size_t initial_capacity = 8;
    static const size_t grow_factor = 2;

public:
    using ValueType = T;

    IndexMap() = default;

    IndexMap(const IndexMap&) = default;
    IndexMap& operator=(const IndexMap&) = default;

    IndexMap(IndexMap&&) = default;
    IndexMap& operator=(IndexMap&&) = default;

    T& operator[](size_t index)
        { return data_[index]; }

    const T& operator[](size_t index) const
        { return data_[index]; }

    size_t Insert(T value)
    {
        if (IsAtFullCapacity())
        {
            Grow();
            return InsertAtBack(std::move(value));
        }
        else if (HasFreeIndices())
        {
            return InsertAtFreeIndex(std::move(value));
        }
        else
        {
            return InsertAtBack(std::move(value));
        }
    }

    void Remove(size_t index)
    {
        for (size_t index : freeIndices_)
            data_[index].~T();
        --size_;
        freeIndices_.push_back(index);
    }

    void Clear()
    {
        // Sort free indexes so we can remove check for them in linear time
        std::sort(freeIndices_.begin(), freeIndices_.end());
        
        const size_t totalSize = size_ + freeIndices_.size();
        size_t i = 0;

        // Skip over free indices
        for (auto freeIndex : freeIndices_)
        {
            for (; i < totalSize; ++i)
            {
                if (i == freeIndex)
                    break;
                else
                    data_[i].~T();
            }
        }

        // Rest
        for (; i < totalSize; ++i)
            data_[i].~T();

        size_ = 0;
        freeIndices_.clear();
    }

    void Reset()
    {
        Clear();

        capacity_ = 0;
        delete[] data_;

        freeIndices_.shrink_to_fit();
    }

    ~IndexMap()
        { Reset(); }

private:
    bool IsAtFullCapacity() const
        { return capacity_ == size_; }

    bool HasFreeIndices() const
        { return !freeIndices_.empty(); }

    size_t CalcNextCapacity() const
        { return capacity_ == 0? initial_capacity : capacity_ * grow_factor;  }

    void Grow()
    {
        // Allocate new storage
        size_t  newCapacity = CalcNextCapacity();
        T*      newData = new T[newCapacity];

        // Move data to new storage
        for (size_t i = 0; i < size_; ++i)
        {
            new (&newData[i]) T(std::move(data_[i]));
            data_[i].~T();
        }
            
        delete[] data_;

        // Use new storage
        data_ = newData;
        capacity_ = newCapacity;
    }

    size_t InsertAtBack(T&& value)
    {
        new (&data_[size_]) T(std::move(value));
        return size_++;
    }

    size_t InsertAtFreeIndex(T&& value)
    {
        size_t nextFreeIndex = freeIndices_.back();
        freeIndices_.pop_back();

        new (&data_[nextFreeIndex]) T(std::move(value));
        ++size_;

        return nextFreeIndex;
    }

    T*      data_       = nullptr;
    size_t  size_       = 0;
    size_t  capacity_   = 0;

    std::vector<size_t> freeIndices_;
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