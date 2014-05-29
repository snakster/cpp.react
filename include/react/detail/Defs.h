
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_DEFS_H_INCLUDED
#define REACT_DETAIL_DEFS_H_INCLUDED

#pragma once

#include <cassert>
#include <cstddef>

///////////////////////////////////////////////////////////////////////////////////////////////////
#define REACT_BEGIN     namespace react {
#define REACT_END       }
#define REACT           ::react

#define REACT_IMPL_BEGIN    REACT_BEGIN     namespace impl {
#define REACT_IMPL_END      REACT_END       }
#define REACT_IMPL          REACT           ::impl

#ifdef _DEBUG
#define REACT_MESSAGE(...) printf(__VA_ARGS__)
#else
#define REACT_MESSAGE
#endif

// Assert with message
#define REACT_ASSERT(condition, ...) for (; !(condition); assert(condition)) printf(__VA_ARGS__)
#define REACT_ERROR(...)    REACT_ASSERT(false, __VA_ARGS__)

// Logging
#ifdef REACT_ENABLE_LOGGING
    #define REACT_LOG(...) __VA_ARGS__
#else
    #define REACT_LOG(...)
#endif

// Thread local storage
#if _WIN32 || _WIN64
    // MSVC
    #define REACT_TLS   __declspec(thread)
#elif __GNUC__
    // GCC
    #define REACT_TLS   __thread    
#else
    // Standard C++11
    #define REACT_TLS   thread_local
#endif

/***************************************/ REACT_IMPL_BEGIN /**************************************/

// Type aliases
using uint = unsigned int;
using uchar = unsigned char;
using std::size_t;

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_DEFS_H_INCLUDED