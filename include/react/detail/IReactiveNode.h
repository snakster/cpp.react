
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_IREACTIVENODE_H_INCLUDED
#define REACT_DETAIL_IREACTIVENODE_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IReactiveNode
///////////////////////////////////////////////////////////////////////////////////////////////////
struct IReactiveNode
{
    virtual ~IReactiveNode() = default;

    /// Returns unique type identifier
    virtual const char* GetNodeType() const = 0;

    // Note: Could get rid of this ugly ptr by adding a template parameter to the interface
    // But that would mean all engine nodes need that template parameter too - so rather cast
    virtual void    Tick(void* turnPtr) = 0;  

    /// Input nodes can be manipulated externally.
    virtual bool    IsInputNode() const = 0;

    /// Output nodes can't have any successors.
    virtual bool    IsOutputNode() const = 0;

    /// Dynamic nodes may change in topology as a result of tick.
    virtual bool    IsDynamicNode() const = 0;

    // Number of predecessors.
    // This information is statically available at compile time on the graph layer,
    // so the engine does not have to calculate it again.
    virtual int     DependencyCount() const = 0;

    // Heavyweight nodes are worth parallelizing.
    virtual bool    IsHeavyweight() const = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IInputNode
///////////////////////////////////////////////////////////////////////////////////////////////////
struct IInputNode
{
    virtual ~IInputNode() = default;

    virtual bool ApplyInput(void* turnPtr) = 0;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif //REACT_DETAIL_IREACTIVENODE_H_INCLUDED