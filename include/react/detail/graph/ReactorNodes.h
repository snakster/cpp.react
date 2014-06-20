
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
    public ReactiveNode<D,void,void>
{
    using Engine = typename ReactorNode::Engine;

public:
    using CoroutineT = boost::coroutines::coroutine<NodeBase<D>*>;
    using PullT = typename CoroutineT::pull_type;
    using PushT = typename CoroutineT::push_type;
    using TurnT = typename D::Engine::TurnT;

    template <typename F>
    ReactorNode(F&& func) :
        ReactorNode::ReactiveNode( ),
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

        assert(p != nullptr);

        Engine::OnNodeAttach(*this, *p);

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
            Engine::OnDynamicNodeAttach(*this, *p, *turnPtr_);
            ++depCount_;
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

        assert(turnPtr_ != nullptr);

        Engine::OnDynamicNodeDetach(*this, *events, *turnPtr_);
        --depCount_;

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
            Engine::OnNodeAttach(*this, *eventsPtr);
            ++depCount_;
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

    //template <typename S>
    //S& Get(const std::shared_ptr<SignalNode<D,S>>& sig)
    //{
    //    // Attach to target signal node
    //    (*curOutPtr_)(sig.get());

    //    Engine::OnDynamicNodeDetach(*this, *events, *turnPtr_);
    //    --depCount_;

    //    auto index = offsets_[reinterpret_cast<uintptr_t>(events.get())]++;
    //    return events->Events()[index];
    //}

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

    std::function<void(TContext)>    func_;

    PullT   mainLoop_;
    TurnT*  turnPtr_;

    PushT*  curOutPtr_ = nullptr;

    uint    depCount_ = 0;

    std::unordered_map<std::uintptr_t, size_t>    offsets_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_GRAPH_REACTORNODES_H_INCLUDED