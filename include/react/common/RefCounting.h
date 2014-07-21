
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_COMMON_REF_COUNTING_H_INCLUDED
#define REACT_COMMON_REF_COUNTING_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <condition_variable>
#include <mutex>

/***************************************/ REACT_IMPL_BEGIN /**************************************/

// A non-exception-safe pointer wrapper with conditional intrusive ref counting.
template <typename T>
class IntrusiveRefCountingPtr
{
    enum
    {
        tag_ref_counted = 0x1
    };

public:
    IntrusiveRefCountingPtr() :
        ptrData_( reinterpret_cast<uintptr_t>(nullptr) )
    {}

    IntrusiveRefCountingPtr(T* ptr) :
        ptrData_( reinterpret_cast<uintptr_t>(ptr) )
    {
        if (ptr != nullptr && ptr->IsRefCounted())
        {
            ptr->IncRefCount();
            ptrData_ |= tag_ref_counted;
        }
    }

    IntrusiveRefCountingPtr(const IntrusiveRefCountingPtr& other) :
        ptrData_( other.ptrData_ )
    {
        if (isValidRefCountedPtr())
            Get()->IncRefCount();
    }

    IntrusiveRefCountingPtr(IntrusiveRefCountingPtr&& other) :
        ptrData_( other.ptrData_ )
    {
        other.ptrData_ = reinterpret_cast<uintptr_t>(nullptr);
    }

    ~IntrusiveRefCountingPtr()
    {
        if (isValidRefCountedPtr())
            Get()->DecRefCount();
    }

    IntrusiveRefCountingPtr& operator=(const IntrusiveRefCountingPtr& other)
    {
        if (this != &other)
        {
            if (other.isValidRefCountedPtr())
                other.Get()->IncRefCount();

            if (isValidRefCountedPtr())
                Get()->DecRefCount();

            ptrData_ = other.ptrData_;
        }

        return *this;
    }

    IntrusiveRefCountingPtr& operator=(IntrusiveRefCountingPtr&& other)
    {
        if (this != &other)
        {
            if (isValidRefCountedPtr())
                Get()->DecRefCount();

            ptrData_ = other.ptrData_;
            other.ptrData_ = reinterpret_cast<uintptr_t>(nullptr);
        }

        return *this;
    }

    T* Get() const          { return reinterpret_cast<T*>(ptrData_ & ~tag_ref_counted); }

    T& operator*() const    { return *Get(); }
    T* operator->() const   { return Get(); }

    bool operator==(const IntrusiveRefCountingPtr& other) const { return Get() == other.Get(); }
    bool operator!=(const IntrusiveRefCountingPtr& other) const { return !(*this == other); }

private:
    bool isValidRefCountedPtr() const
    {
        return ptrData_ != reinterpret_cast<uintptr_t>(nullptr)
            && (ptrData_ & tag_ref_counted) != 0;
    }

    uintptr_t   ptrData_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_COMMON_REF_COUNTING_H_INCLUDED