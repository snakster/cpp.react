
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/Defs.h"

#include <atomic>
#include <memory>
#include <functional>

#include "react/common/Util.h"
#include "react/common/Types.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// NodeBase
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class NodeBase :
    public std::enable_shared_from_this<NodeBase<D>>,
    public D::Policy::Engine::NodeInterface
{
public:
    using PtrT = std::shared_ptr<NodeBase>;

    using Domain = D;
    using Policy = typename D::Policy;
    using Engine = typename D::Engine;
    using NodeInterface = typename Engine::NodeInterface;
    using TurnInterface = typename Engine::TurnInterface;

    virtual const char* GetNodeType() const override { return "NodeBase"; }
    
    virtual bool    IsInputNode() const override    { return false; }
    virtual bool    IsOutputNode() const override   { return false; }
    virtual bool    IsDynamicNode() const override  { return false; }

    virtual int     DependencyCount() const      { return 0; }

    PtrT GetSharedPtr() const
    {
        return shared_from_this();
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ReactiveNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename P,
    typename V
>
class ReactiveNode : public NodeBase<D>
{
public:
    using PtrT = std::shared_ptr<ReactiveNode>;

    explicit ReactiveNode(bool registered) :
        obsCount_(0)
    {
        if (!registered)
            registerNode();
    }

    ~ReactiveNode()
    {
        if (GetObsCount() > 0)
            D::Observers().UnregisterFrom(this);

        Engine::OnNodeDestroy(*this);
    }

    virtual const char* GetNodeType() const override { return "ReactiveNode"; }

    void    IncObsCount()       { obsCount_.fetch_add(1, std::memory_order_relaxed); }
    void    DecObsCount()       { obsCount_.fetch_sub(1, std::memory_order_relaxed); }
    uint    GetObsCount() const { return obsCount_.load(std::memory_order_relaxed); }

protected:
    void registerNode()
    {
        Engine::OnNodeCreate(*this);
    }

private:
    std::atomic<uint>    obsCount_;
};

/****************************************/ REACT_IMPL_END /***************************************/
