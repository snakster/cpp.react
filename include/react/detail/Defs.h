
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_DEFS_H_INCLUDED
#define REACT_DETAIL_DEFS_H_INCLUDED

#pragma once

#include <cstddef>

///////////////////////////////////////////////////////////////////////////////////////////////////
#define REACT_BEGIN     namespace react {
#define REACT_END       }
#define REACT           ::react

#define REACT_IMPL_BEGIN    REACT_BEGIN     namespace impl {
#define REACT_IMPL_END      REACT_END       }
#define REACT_IMPL          REACT           ::impl

/*****************************************/ REACT_BEGIN /*****************************************/

// Type aliases
using uint = unsigned int;
using uchar = unsigned char;
using std::size_t;

/******************************************/ REACT_END /******************************************/

#endif // REACT_DETAIL_DEFS_H_INCLUDED