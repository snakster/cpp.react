
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/Defs.h"

#include <atomic>
#include <algorithm>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

#include "tbb/spin_mutex.h"

#include "GraphBase.h"
#include "react/common/Types.h"


/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventStreamNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename E
>
class EventStreamNode : public ReactiveNode<D,E,void>
{
public:
    using EventListT    = std::vector<E>;
    using EventMutexT   = tbb::spin_mutex;

    using PtrT      = std::shared_ptr<EventStreamNode>;
    using WeakPtrT  = std::weak_ptr<EventStreamNode>;

    using EngineT   = typename D::Engine;
    using TurnT     = typename EngineT::TurnT;

    explicit EventStreamNode(bool registered) :
        ReactiveNode(true)
    {
        if (!registered)
            registerNode();
    }

    virtual const char* GetNodeType() const override    { return "EventStreamNode"; }

    void SetCurrentTurn(const TurnT& turn, bool forceUpdate = false, bool noClear = false)
    {// eventMutex_
        EventMutexT::scoped_lock lock(eventMutex_);

        if (curTurnId_ != turn.Id() || forceUpdate)
        {
            curTurnId_ =  turn.Id();
            if (!noClear)
                events_.clear();
        }
    }// ~eventMutex_

    EventListT&     Events()        { return events_; }
    void            ClearEvents()   { events_.clear(); }

protected:
    EventListT    events_;
    EventMutexT   eventMutex_;

    uint    curTurnId_ = INT_MAX;
};

template <typename D, typename E>
using EventStreamNodePtr = typename EventStreamNode<D,E>::PtrT;

template <typename D, typename E>
using EventStreamNodeWeakPtr = typename EventStreamNode<D,E>::WeakPtrT;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventSourceNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename E
>
class EventSourceNode : public EventStreamNode<D,E>
{
public:
    explicit EventSourceNode(bool registered) :
        EventStreamNode(true)
    {
        if (!registered)
            registerNode();
    }

    virtual const char* GetNodeType() const override    { return "EventSourceNode"; }

    virtual ETickResult Tick(void* turnPtr) override
    {
        if (events_.size() > 0 && !changedFlag_)
        {
            using TurnT = typename D::Engine::TurnT;
            TurnT& turn = *static_cast<TurnT*>(turnPtr);

            SetCurrentTurn(turn, true, true);
            changedFlag_ = true;
            Engine::OnTurnInputChange(*this, turn);
            return ETickResult::pulsed;
        }
        else
        {
            return ETickResult::none;
        }
    }

    virtual bool IsInputNode() const override    { return true; }

    template <typename V>
    void AddInput(V&& v)
    {
        // Clear input from previous turn
        if (changedFlag_)
        {
            changedFlag_ = false;
            events_.clear();
        }

        events_.push_back(std::forward<V>(v));
    }

private:
    bool changedFlag_ = false;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventMergeNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename E,
    typename ... TArgs
>
class EventMergeNode : public EventStreamNode<D, E>
{
public:
    EventMergeNode(const EventStreamNodePtr<D, TArgs>& ... args, bool registered) :
        EventStreamNode<D, E>(true),
        deps_{ make_tuple(args ...) },
        func_{ std::bind(&EventMergeNode::expand, this, std::placeholders::_1, args ...) }
    {
        if (!registered)
            registerNode();

        REACT_EXPAND_PACK(Engine::OnNodeAttach(*this, *args));
    }

    ~EventMergeNode()
    {
        apply
        (
            [this] (const EventStreamNodePtr<D, TArgs>& ... args)
            {
                REACT_EXPAND_PACK(Engine::OnNodeDetach(*this, *args));
            },
            deps_
        );
    }

    virtual const char* GetNodeType() const override    { return "EventMergeNode"; }

    virtual ETickResult Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *static_cast<TurnT*>(turnPtr);

        //printf("EventMergeNode: Tick %08X by thread %08X\n", this, std::this_thread::get_id().hash());

        SetCurrentTurn(turn, true);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        func_(std::cref(turn));

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (events_.size() > 0)
        {
            Engine::OnNodePulse(*this, *static_cast<TurnT*>(turnPtr));
            return ETickResult::pulsed;
        }
        else
        {
            Engine::OnNodeIdlePulse(*this, *static_cast<TurnT*>(turnPtr));
            return ETickResult::idle_pulsed;
        }
    }

    virtual int DependencyCount() const override    { return sizeof... (TArgs); }

private:
    const std::tuple<EventStreamNodePtr<D, TArgs> ...>  deps_;
    const std::function<void(const TurnT&)>     func_;

    inline void expand(const TurnT& turn, const EventStreamNodePtr<D, TArgs>& ... args)
    {
        REACT_EXPAND_PACK(processArgs<TArgs>(turn, args));
    }

    template <typename TArg>
    inline void processArgs(const TurnT& turn, const EventStreamNodePtr<D, TArg>& arg)
    {
        arg->SetCurrentTurn(turn);

        events_.insert(events_.end(), arg->Events().begin(), arg->Events().end()); // TODO
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventFilterNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename E
>
class EventFilterNode : public EventStreamNode<D, E>
{
public:
    template <typename F>
    EventFilterNode(const EventStreamNodePtr<D, E>& src, F&& filter, bool registered) :
        EventStreamNode<D, E>(true),
        src_{ src },
        filter_{ std::forward<F>(filter) }
    {
        if (!registered)
            registerNode();

        Engine::OnNodeAttach(*this, *src_);
    }

    ~EventFilterNode()
    {
        Engine::OnNodeDetach(*this, *src_);
    }

    virtual const char* GetNodeType() const override    { return "EventFilterNode"; }

    virtual ETickResult Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *static_cast<TurnT*>(turnPtr);

        SetCurrentTurn(turn, true);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        std::copy_if(src_->Events().begin(), src_->Events().end(), std::back_inserter(events_), filter_);

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (events_.size() > 0)
        {
            Engine::OnNodePulse(*this, *static_cast<TurnT*>(turnPtr));
            return ETickResult::pulsed;
        }
        else
        {
            Engine::OnNodeIdlePulse(*this, *static_cast<TurnT*>(turnPtr));
            return ETickResult::idle_pulsed;
        }
    }

    virtual int DependencyCount() const override    { return 1; }

private:
    const EventStreamNodePtr<D,E>         src_;
    const std::function<bool(const E&)>   filter_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventTransformNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename TIn,
    typename TOut
>
class EventTransformNode : public EventStreamNode<D,TOut>
{
public:
    template <typename F>
    EventTransformNode(const EventStreamNodePtr<D,TIn>& src, F&& func, bool registered) :
        EventStreamNode<D,TOut>(true),
        src_{ src },
        func_{ std::forward<F>(func) }
    {
        if (!registered)
            registerNode();

        Engine::OnNodeAttach(*this, *src_);
    }

    ~EventTransformNode()
    {
        Engine::OnNodeDetach(*this, *src_);
    }

    virtual const char* GetNodeType() const override { return "EventTransformNode"; }

    virtual ETickResult Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *static_cast<TurnT*>(turnPtr);

        SetCurrentTurn(turn, true);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        std::transform(src_->Events().begin(), src_->Events().end(), std::back_inserter(events_), func_);    

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (events_.size() > 0)
        {
            Engine::OnNodePulse(*this, *static_cast<TurnT*>(turnPtr));
            return ETickResult::pulsed;
        }
        else
        {
            Engine::OnNodeIdlePulse(*this, *static_cast<TurnT*>(turnPtr));
            return ETickResult::idle_pulsed;
        }
    }

    virtual int DependencyCount() const override    { return 1; }

private:
    const EventStreamNodePtr<D,TIn>         src_;
    const std::function<TOut(const TIn&)>   func_;
};

/****************************************/ REACT_IMPL_END /***************************************/
