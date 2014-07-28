
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_REACTOR_H_INCLUDED
#define REACT_REACTOR_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <functional>
#include <memory>
#include <utility>

#include "react/common/Util.h"

#include "react/Event.h"
#include "react/detail/ReactiveBase.h"
#include "react/detail/graph/ReactorNodes.h"

/*****************************************/ REACT_BEGIN /*****************************************/

template <typename D>
class Reactor
{
public:
    class Context;

    using NodeT = REACT_IMPL::ReactorNode<D,Context>;
    
    class Context
    {
    public:
        Context(NodeT& node) :
            node_( node )
        {}

        template <typename E>
        E& Await(const Events<D,E>& evn)
        {
            return node_.Await(GetNodePtr(evn));
        }

        template <typename E, typename F>
        void RepeatUntil(const Events<D,E>& evn, F&& func)
        {
            node_.RepeatUntil(GetNodePtr(evn), std::forward<F>(func));
        }

        template <typename S>
        const S& Get(const Signal<D,S>& sig)
        {
            return node_.Get(GetNodePtr(sig));
        }

    private:
        NodeT&    node_;
    };

    template <typename F>
    explicit Reactor(F&& func) :
        nodePtr_( new REACT_IMPL::ReactorNode<D, Context>(std::forward<F>(func)) )
    {
        nodePtr_->StartLoop();
    }

private:
    std::unique_ptr<NodeT>    nodePtr_;
};

/******************************************/ REACT_END /******************************************/

#endif // REACT_REACTOR_H_INCLUDED