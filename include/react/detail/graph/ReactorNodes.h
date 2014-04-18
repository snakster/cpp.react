
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#ifndef REACT_DISABLE_REACTORS

#include "react/detail/Defs.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>

#include <boost/coroutine/all.hpp>

#include "GraphBase.h"
#include "EventStreamNodes.h"

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
public:
    using NodeBasePtrT = NodeBase<D>::PtrT;

    using CoroutineT = boost::coroutines::coroutine<const NodeBasePtrT*>;
    using LoopT = typename CoroutineT::pull_type;
    using OutT = typename CoroutineT::push_type;
    
    using TurnT = typename D::Engine::TurnT;

    template <typename F>
    ReactorNode(F&& func) :
        ReactiveNode<D,void,void>(),
        func_{ std::forward<F>(func) }
    {
        Engine::OnNodeCreate(*this);
    }

    ~ReactorNode()
    {
        Engine::OnNodeDestroy(*this);
    }

    void StartLoop()
    {
        // Could already have started it in ctor,
        // but lets make sure node is fully constructed before calls to
        // context in coroutine can happen.
        mainLoop_ = LoopT
        (
            [&] (OutT& out)
            {
                curOutPtr_ = &out;

                TContext ctx{ *this };

                while (true)
                {
                    func_(ctx);
                }
            }
        );

        // First blocking event is not initiated by Tick() but after loop creation.
        const auto* p = mainLoop_.get();

        REACT_ASSERT(p != nullptr, "StartLoop: first depPtr was null");

        Engine::OnNodeAttach(*this, **p);
        ++depCount_;
    }

    virtual const char* GetNodeType() const override { return "ReactorNode"; }

    virtual bool IsDynamicNode() const override { return true; }
    virtual bool IsOutputNode() const override  { return true; }

    virtual EUpdateResult Tick(void* turnPtr) override
    {
        turnPtr_ = static_cast<TurnT*>(turnPtr);
        REACT_SCOPE_EXIT{ turnPtr_ = nullptr; };

        mainLoop_();

        if (mainLoop_.get() != nullptr)
        {
            const auto& depPtr = *mainLoop_.get();
            Engine::OnDynamicNodeAttach(*this, *depPtr, *turnPtr_);
            ++depCount_;
            
            return EUpdateResult::invalidated;
        }

        offsets_.clear();

        return EUpdateResult::none;
    }

    virtual int DependencyCount() const override
    {
        return depCount_;
    }

    template <typename E>
    E& Await(const EventStreamNodePtr<D,E>& events)
    {
        // First attach to target event node
        (*curOutPtr_)(&std::static_pointer_cast<NodeBase<D>>(events));

        while (! checkEvent<E>(events))
            (*curOutPtr_)(nullptr);

        REACT_ASSERT(turnPtr_ != nullptr, "Take: turnPtr_ was null");

        Engine::OnDynamicNodeDetach(*this, *events, *turnPtr_);
        --depCount_;

        return events->Events()[offsets_[reinterpret_cast<uintptr_t>(&events)]++];
    }

    template <typename E, typename F>
    void RepeatUntil(const EventStreamNodePtr<D,E>& events, F func)
    {
        // First attach to target event node
        if (turnPtr_ != nullptr)
        {
            (*curOutPtr_)(&std::static_pointer_cast<NodeBase<D>>(events));
        }
        else
        {
            // Non-dynamic attach in case first loop until is encountered before the loop was
            // suspended for the first time.
            Engine::OnNodeAttach(*this, *events);
            ++depCount_;
        }

        // Detach when this function is exited
        REACT_SCOPE_EXIT
        {
            if (turnPtr_ != nullptr)
                Engine::OnDynamicNodeDetach(*this, *events, *turnPtr_);
            else
                Engine::OnNodeDetach(*this, *events);
            --depCount_;
        };

        // Don't enter loop if event already present
        if (checkEvent<E>(events))
            return;

        auto* parentOutPtr = curOutPtr_;
        REACT_SCOPE_EXIT{ curOutPtr_ = parentOutPtr; };

        // Create and start loop
        LoopT nestedLoop_
        {
            [&] (OutT& out)
            {
                curOutPtr_ = &out;
                while (true)
                    func();
            }
        };

        // First suspend from initial loop run
        (*parentOutPtr)(nestedLoop_.get());

        // Further iterations
        while (! checkEvent<E>(events))
        {
            // Advance loop, forward blocking event to parent, and suspend
            nestedLoop_(); 
            (*parentOutPtr)(nestedLoop_.get());
        }
    }

private:
    template <typename E>
    bool checkEvent(const EventStreamNodePtr<D,E>& events)
    {
        if (turnPtr_ == nullptr)
            return false;

        events->SetCurrentTurn(*turnPtr_);
        return offsets_[reinterpret_cast<uintptr_t>(&events)] < events->Events().size();
    }

    std::function<void(TContext&)>    func_;

    LoopT   mainLoop_;
    TurnT*  turnPtr_;

    OutT*   curOutPtr_ = nullptr;

    int depCount_ = 0;

    std::unordered_map<std::uintptr_t, std::size_t>    offsets_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif //REACT_DISABLE_REACTORS