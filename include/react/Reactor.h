
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#ifdef REACT_ENABLE_REACTORS

#include "react/detail/Defs.h"

#include <functional>
#include <memory>

#include "react/common/Util.h"

#include "react/Event.h"
#include "react/detail/ReactiveBase.h"
#include "react/detail/graph/ReactorNodes.h"

/*****************************************/ REACT_BEGIN /*****************************************/

template <typename D>
class ReactiveLoop
{
public:
    class Context;

    using NodeT = REACT_IMPL::ReactorNode<D,Context>;
    
    class Context
    {
    public:
        Context(NodeT& node) :
            node_{ node }
        {}

        template <typename E>
        E& Await(const Events<D,E>& evn)
        {
            return node_.Await<E>(evn.NodePtr());
        }

        template <typename E, typename F>
        void RepeatUntil(const Events<D,E>& evn, F func)
        {
            node_.RepeatUntil<E>(evn.NodePtr(), func);
        }

    private:
        NodeT&    node_;
    };

    template <typename F>
    ReactiveLoop(F&& func) :
        nodePtr_{ new REACT_IMPL::ReactorNode<D, Context>(std::forward<F>(func)) }
    {
        nodePtr_->StartLoop();
    }

private:
    std::unique_ptr<NodeT>    nodePtr_;
};

/******************************************/ REACT_END /******************************************/

#endif //REACT_ENABLE_REACTORS