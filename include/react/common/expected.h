
//          Copyright Sebastian Jeckel 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_COMMON_EXPECTED_H_INCLUDED
#define REACT_COMMON_EXPECTED_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <string>
#include <sstream>
#include <memory>
#include <type_traits>

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IError
///////////////////////////////////////////////////////////////////////////////////////////////////
class IErrorCause
{
public:
    virtual ~IErrorCause() = default;

    virtual std::string GetMessage() const = 0;

    virtual bool IsOfType(const char* typeId) const = 0;
};


class AllocationError : public IErrorCause
{
public:
    static constexpr const char* type_id = "react::AllocationError";

    virtual std::string GetMessage() const
        { return "Allocation error."; }

    virtual bool IsOfType(const char* typeId) const
        { return typeId == type_id; }
};


class PreconditionError : public IErrorCause
{
public:
    static constexpr const char* type_id = "react::PreconditionError";

    virtual std::string GetMessage() const
        { return "Precondition error."; }

    virtual bool IsOfType(const char* typeId) const
        { return typeId == type_id; }
};


class PostconditionError : public IErrorCause
{
public:
    static constexpr const char* type_id = "react::PostconditionError";

    virtual std::string GetMessage() const
        { return "Postcondition error."; }

    virtual bool IsOfType(const char* typeId) const
        { return typeId == type_id; }
};


class MissingValueError : public IErrorCause
{
public:
    static constexpr const char* type_id = "react::MissingValueError";

    virtual std::string GetMessage() const
        { return "Missing value error."; }

    virtual bool IsOfType(const char* typeId) const
        { return typeId == type_id; }
};


class Error
{
public:
    template
    <
        typename T,
        typename = std::enable_if_t<std::is_base_of_v<IErrorCause, T>>
    >
    Error(T cause) :
        cause_(std::make_shared<T>(std::move(cause)))
    { }

    template
    <
        typename T,
        typename = std::enable_if_t<std::is_base_of_v<IErrorCause, T>>
    >
    bool IsCause() const
        { return cause_->IsOfType(T::type_id); }

    template
    <
        typename T,
        typename = std::enable_if_t<std::is_base_of_v<IErrorCause, T>>
    >
    T* GetCause() const
        { return static_cast<T*>(cause_.get()); }

    std::string GetMessage() const
        { return cause_->GetMessage(); }

private:
    std::shared_ptr<const IErrorCause> cause_;
};

static Error* GetErrorSentinel()
    { return reinterpret_cast<Error*>(0x1u); }

struct ErrorDeleter
{
    void operator()(Error* p) const
    {
        if (p != GetErrorSentinel())
            delete p;
    }
};

using UniqueErrorPtrType = std::unique_ptr<Error, ErrorDeleter>;


///////////////////////////////////////////////////////////////////////////////////////////////////
/// Expected
///
/// TODO: Optimize for different storage types to avoid branch in destructor.
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
class Expected
{
public:
    Expected() = delete;

    Expected(const Expected&) = delete;

    Expected& operator=(const Expected&) = delete;

    Expected(Expected&&) = default;

    Expected& operator=(Expected&&) = default;

    Expected(const T& value) :
        error_( GetErrorSentinel() )
    {
        ValueRef() = value;
    }

    Expected(T&& value) :
        error_( GetErrorSentinel() )
    {
        ValueRef() = std::move(value);
    }

    Expected(Error err) :
        error_(new Error(std::move(err)))
    { }

    Expected(UniqueErrorPtrType&& err) :
        error_(std::move(err))
    { }

    ~Expected()
    {
        if (error_.get() == GetErrorSentinel())
        {
            Destruct();
        }   
    }

    explicit operator bool() const
    {
        return error_.get() == GetErrorSentinel();
    }

    Error GetError()
        { return *error_; }

private:
    const T& ValueRef() const
        { return *reinterpret_cast<T*>(&valueStorage_); }

    T& ValueRef()
        { return *reinterpret_cast<T*>(&valueStorage_); }

    void Destruct()
        { reinterpret_cast<const T*>(&valueStorage_)->~T(); }

    std::aligned_storage_t<sizeof(T)> valueStorage_;

    std::unique_ptr<Error, ErrorDeleter> error_;

    template <typename U>
    friend typename UniqueErrorPtrType UnwindExpected(Expected<U>&& ex);
};

template <typename T>
typename UniqueErrorPtrType UnwindExpected(Expected<T>&& ex)
{
    return std::move(std::move(ex).error_);
}

/******************************************/ REACT_END /******************************************/

#endif // REACT_COMMON_INDEXED_STORAGE_H_INCLUDED