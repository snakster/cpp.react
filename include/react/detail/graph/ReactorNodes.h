
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_GRAPH_REACTORNODES_H_INCLUDED
#define REACT_DETAIL_GRAPH_REACTORNODES_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>

#include <boost/coroutine/all.hpp>

#include "GraphBase.h"
#include "EventNodes.h"
#include "SignalNodes.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ReactorNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename TContext
>
class ReactorNode :
    public NodeBase<D>
{
    using Engine = typename ReactorNode::Engine;

public:
    using CoroutineT = boost::coroutines::coroutine<NodeBase<D>*>;
    using PullT = typename CoroutineT::pull_type;
    using PushT = typename CoroutineT::push_type;
    using TurnT = typename D::Engine::TurnT;

    template <typename F>
    ReactorNode(F&& func) :
        ReactorNode::NodeBase( ),
        func_( std::forward<F>(func) )
    {
        Engine::OnNodeCreate(*this);
    }

    ~ReactorNode()
    {
        Engine::OnNodeDestroy(*this);
    }

    void StartLoop()
    {
        mainLoop_ = PullT
        (
            [&] (PushT& out)
            {
                curOutPtr_ = &out;

                TContext ctx( *this );

                // Start the loop function.
                // It will reach it its first Await at some point.
                while (true)
                {
                    func_(ctx);
                }
            }
        );

        // First blocking event is not initiated by Tick() but after loop creation.
        NodeBase<D>* p = mainLoop_.get();
        doAttach(p);

        ++depCount_;
    }

    virtual const char* GetNodeType() const override { return "ReactorNode"; }

    virtual bool IsDynamicNode() const override { return true; }
    virtual bool IsOutputNode() const override  { return true; }

    virtual void Tick(void* turnPtr) override
    {
        turnPtr_ = reinterpret_cast<TurnT*>(turnPtr);

        mainLoop_();

        NodeBase<D>* p = mainLoop_.get();

        if (p != nullptr)
        {
            doDynAttach(p);
        }
        else
        {
            offsets_.clear();
        }
        
        turnPtr_ = nullptr;
    }

    virtual int DependencyCount() const override
    {
        return depCount_;
    }

    template <typename E>
    E& Await(const std::shared_ptr<EventStreamNode<D,E>>& events)
    {
        // First attach to target event node
        (*curOutPtr_)(events.get());

        while (! checkEvent(events))
            (*curOutPtr_)(nullptr);

        doDynDetach(events.get());

        auto index = offsets_[reinterpret_cast<uintptr_t>(events.get())]++;
        return events->Events()[index];
    }

    template <typename E, typename F>
    void RepeatUntil(const std::shared_ptr<EventStreamNode<D,E>>& eventsPtr, const F& func)
    {
        // First attach to target event node
        if (turnPtr_ != nullptr)
        {
            (*curOutPtr_)(eventsPtr.get());
        }
        else
        {
            // Non-dynamic attach in case first loop until is encountered before the loop was
            // suspended for the first time.
            doAttach(eventsPtr.get());
        }

        // Don't enter loop if event already present
        if (! checkEvent(eventsPtr))
        {
            auto* parentOutPtr = curOutPtr_;

            // Create and start loop
            PullT nestedLoop_
            {
                [&] (PushT& out)
                {
                    curOutPtr_ = &out;
                    while (true)
                        func();
                }
            };

            // First suspend from initial loop run
            (*parentOutPtr)(nestedLoop_.get());

            // Further iterations
            while (! checkEvent(eventsPtr))
            {
                // Advance loop, forward blocking event to parent, and suspend
                nestedLoop_(); 
                (*parentOutPtr)(nestedLoop_.get());
            }

            curOutPtr_ = parentOutPtr;
        }

        // Detach when this function is exited
        if (turnPtr_ != nullptr)
            Engine::OnDynamicNodeDetach(*this, *eventsPtr, *turnPtr_);
        else
            Engine::OnNodeDetach(*this, *eventsPtr);

        --depCount_;
    }

    template <typename S>
    const S& Get(const std::shared_ptr<SignalNode<D,S>>& sigPtr)
    {
        // Do dynamic attach followed by attach if Get() happens during a turn.
        if (turnPtr_ != nullptr)
        {
            // Request attach
            (*curOutPtr_)(sigPtr.get());

            doDynDetach(sigPtr.get());
        }

        return sigPtr->ValueRef();
    }

private:
    template <typename E>
    bool checkEvent(const std::shared_ptr<EventStreamNode<D,E>>& events)
    {
        if (turnPtr_ == nullptr)
            return false;

        events->SetCurrentTurn(*turnPtr_);

        auto index = reinterpret_cast<uintptr_t>(events.get());
        return offsets_[index] < events->Events().size();
    }

    void doAttach(NodeBase<D>* nodePtr)
    {
        assert(nodePtr != nullptr);
        assert(turnPtr_ == nullptr);
        Engine::OnNodeAttach(*this, *nodePtr);
        ++depCount_;
    }

    void doDetach(NodeBase<D>* nodePtr)
    {
        assert(nodePtr != nullptr);
        assert(turnPtr_ == nullptr);
        Engine::OnNodeDetach(*this, *nodePtr);
        --depCount_;
    }

    void doDynAttach(NodeBase<D>* nodePtr)
    {
        assert(nodePtr != nullptr);
        assert(turnPtr_ != nullptr);
        Engine::OnDynamicNodeAttach(*this, *nodePtr, *turnPtr_);
        ++depCount_;
    }

    void doDynDetach(NodeBase<D>* nodePtr)
    {
        assert(nodePtr != nullptr);
        assert(turnPtr_ != nullptr);
        Engine::OnDynamicNodeDetach(*this, *nodePtr, *turnPtr_);
        --depCount_;
    }

    std::function<void(TContext)>    func_;

    PullT   mainLoop_;
    TurnT*  turnPtr_;

    PushT*  curOutPtr_ = nullptr;

    uint    depCount_ = 0;

    std::unordered_map<std::uintptr_t, size_t>    offsets_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_GRAPH_REACTORNODES_H_INCLUDED