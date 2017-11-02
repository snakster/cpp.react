
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_COMMON_SLOTMAP_H_INCLUDED
#define REACT_COMMON_SLOTMAP_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <memory>
#include <type_traits>

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// A simple slot map.
/// Insert returns the slot index, which stays valid until the element is erased.
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
class SlotMap
{
    static const size_t initial_capacity = 8;
    static const size_t grow_factor = 2;

    using StorageType = typename std::aligned_storage<sizeof(T), alignof(T)>::type;

public:
    using ValueType = T;

    SlotMap() = default;

    SlotMap(SlotMap&&) = default;
    SlotMap& operator=(SlotMap&&) = default;

    SlotMap(const SlotMap&) = delete;
    SlotMap& operator=(const SlotMap&) = delete;

    ~SlotMap()
        { Reset(); }

    T& operator[](size_t index)
        { return reinterpret_cast<T&>(data_[index]); }

    const T& operator[](size_t index) const
        { return reinterpret_cast<T&>(data_[index]); }

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

    void Erase(size_t index)
    {
        // If we erased something other than the last element, save in free index list.
        if (index != (size_ - 1))
        {
            freeIndices_[freeSize_++] = index;
        }

        reinterpret_cast<T&>(data_[index]).~T();
        --size_;
    }

    void Clear()
    {
        // Sort free indexes so we can remove check for them in linear time.
        std::sort(&freeIndices_[0], &freeIndices_[freeSize_]);
        
        const size_t totalSize = size_ + freeSize_;
        size_t index = 0;

        // Skip over free indices.
        for (size_t j = 0; j < freeSize_; ++j)
        {
            size_t freeIndex = freeIndices_[j];

            for (; index < totalSize; ++index)
            {
                if (index == freeIndex)
                {
                    ++index;
                    break;
                }
                else
                {
                    reinterpret_cast<T&>(data_[index]).~T();
                }
            }
        }

        // Rest
        for (; index < totalSize; ++index)
            reinterpret_cast<T&>(data_[index]).~T();

        size_ = 0;
        freeSize_ = 0;
    }

    void Reset()
    {
        Clear();

        data_.reset();
        freeIndices_.reset();

        capacity_ = 0;
    }

private:
    T& GetDataAt(size_t index)
        { return reinterpret_cast<T&>(data_[index]); }

    T& GetDataAt(size_t index) const
        { return reinterpret_cast<T&>(data_[index]); }

    bool IsAtFullCapacity() const
        { return capacity_ == size_; }

    bool HasFreeIndices() const
        { return freeSize_ > 0; }

    size_t CalcNextCapacity() const
        { return capacity_ == 0? initial_capacity : capacity_ * grow_factor;  }

    void Grow()
    {
        // Allocate new storage
        size_t  newCapacity = CalcNextCapacity();
        
        std::unique_ptr<StorageType[]> newData{ new StorageType[newCapacity] };
        std::unique_ptr<size_t[]> newFreeIndices{ new size_t[newCapacity] };

        // Move data to new storage
        for (size_t i = 0; i < capacity_; ++i)
        {
            new (reinterpret_cast<T*>(&newData[i])) T{ std::move(reinterpret_cast<T&>(data_[i])) };
            reinterpret_cast<T&>(data_[i]).~T();
        }

        // Free list is empty if we are at max capacity anyway

        // Use new storage
        data_           = std::move(newData);
        freeIndices_    = std::move(newFreeIndices);
        capacity_       = newCapacity;
    }

    size_t InsertAtBack(T&& value)
    {
        new (&data_[size_]) T(std::move(value));
        return size_++;
    }

    size_t InsertAtFreeIndex(T&& value)
    {
        size_t nextFreeIndex = freeIndices_[--freeSize_];
        new (&data_[nextFreeIndex]) T(std::move(value));
        ++size_;

        return nextFreeIndex;
    }

    std::unique_ptr<StorageType[]>  data_;
    std::unique_ptr<size_t[]>       freeIndices_;

    size_t  size_       = 0;
    size_t  freeSize_   = 0;
    size_t  capacity_   = 0;
};

/******************************************/ REACT_END /******************************************/

#endif // REACT_COMMON_SLOTMAP_H_INCLUDED