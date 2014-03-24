
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/Defs.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// TickResult
///////////////////////////////////////////////////////////////////////////////////////////////////
enum class ETickResult
{
    none,
    pulsed,
    idle_pulsed,
    invalidated
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IReactiveNode
///////////////////////////////////////////////////////////////////////////////////////////////////
struct IReactiveNode
{
    virtual ~IReactiveNode()    {}

    // Unique type identifier
    virtual const char* GetNodeType() const = 0;

    virtual ETickResult    Tick(void* turnPtr) = 0;

    /// Input nodes can be manipulated externally.
    virtual bool    IsInputNode() const = 0;

    /// Output nodes can't have any successors.
    virtual bool    IsOutputNode() const = 0;

    /// This node can have successors and may be re-attached to other nodes.
    virtual bool    IsDynamicNode() const = 0;

    virtual int        DependencyCount() const = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IObserverNode
///////////////////////////////////////////////////////////////////////////////////////////////////
struct IObserverNode
{
    virtual ~IObserverNode() {}

    virtual void Detach() = 0;
};

/****************************************/ REACT_IMPL_END /***************************************/
