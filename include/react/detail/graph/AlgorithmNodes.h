
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/detail/Defs.h"

#include <utility>

#include "EventNodes.h"
#include "SignalNodes.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IterateNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S,
    typename E,
    typename TFunc
>
class IterateNode : public SignalNode<D,S>
{
public:
    template <typename T, typename F>
    IterateNode(T&& init, const std::shared_ptr<EventStreamNode<D,E>>& events, F&& func) :
        SignalNode{ std::forward<T>(init) },
        events_{ events },
        func_{ std::forward<F>(func) }
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *events);
    }

    ~IterateNode()
    {
        Engine::OnNodeDetach(*this, *events_);
        Engine::OnNodeDestroy(*this);
    }

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        S newValue = value_;
        for (const auto& e : events_->Events())
            newValue = func_(e, newValue);

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (! impl::Equals(newValue, value_))
        {
            value_ = std::move(newValue);
            Engine::OnNodePulse(*this, turn);
        }
        else
        {
            Engine::OnNodeIdlePulse(*this, turn);
        }
    }

    virtual const char* GetNodeType() const override    { return "IterateNode"; }
    virtual int DependencyCount() const    override     { return 1; }

private:
    SharedPtrT<EventStreamNode<D,E>> events_;
    
    TFunc   func_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IterateByRefNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S,
    typename E,
    typename TFunc
>
class IterateByRefNode : public SignalNode<D,S>
{
public:
    template <typename T, typename F>
    IterateByRefNode(T&& init, const SharedPtrT<EventStreamNode<D,E>>& events, F&& func) :
        SignalNode{ std::forward<T>(init) },
        func_{ std::forward<F>(func) },
        events_{ events }
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *events);
    }

    ~IterateByRefNode()
    {
        Engine::OnNodeDetach(*this, *events_);
        Engine::OnNodeDestroy(*this);
    }

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        for (const auto& e : events_->Events())
            func_(e, value_);

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        // Always assume change
        Engine::OnNodePulse(*this, turn);
    }

    virtual const char* GetNodeType() const override        { return "IterateByRefNode"; }
    virtual int         DependencyCount() const override    { return 1; }

protected:
    TFunc   func_;

    SharedPtrT<EventStreamNode<D,E>> events_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncedIterateNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S,
    typename E,
    typename TFunc,
    typename ... TDepValues
>
class SyncedIterateNode : public SignalNode<D,S>
{
public:
    template <typename T, typename F>
    SyncedIterateNode(T&& init, const std::shared_ptr<EventStreamNode<D,E>>& events, F&& func,
                      const std::shared_ptr<SignalNode<D,TDepValues>>& ... deps) :
        SignalNode{ std::forward<T>(init) },
        events_{ events },
        func_{ std::forward<F>(func) }

    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *events);
        REACT_EXPAND_PACK(Engine::OnNodeAttach(*this, *deps));
    }

    ~SyncedIterateNode()
    {
        Engine::OnNodeDetach(*this, *events_);

        apply(
            DetachFunctor<D,SyncedIterateNode,
                SharedPtrT<SignalNode<D,TDepValues>>...>{ *this },
            deps_);

        Engine::OnNodeDestroy(*this);
    }

    virtual void Tick(void* turnPtr) override
    {
        struct EvalFunctor_
        {
            EvalFunctor_(const E& e, const S& v, TFunc& f) :
                MyEvent{ e },
                MyValue{ v },
                MyFunc{ f }
            {}

            S operator()(const SharedPtrT<SignalNode<D,TDepValues>>& ... args)
            {
                return MyFunc(MyEvent, MyValue, args->ValueRef() ...);
            }

            const E&    MyEvent;
            const S&    MyValue;
            TFunc&      MyFunc;
        };

        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        S newValue = value_;
        for (const auto& e : events_->Events())
            newValue = apply(EvalFunctor_{ e, newValue, func_ }, deps_);

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (! impl::Equals(newValue, value_))
        {
            value_ = std::move(newValue);
            Engine::OnNodePulse(*this, turn);
        }
        else
        {
            Engine::OnNodeIdlePulse(*this, turn);
        }
    }

    virtual const char* GetNodeType() const override        { return "SyncedIterateNode"; }
    virtual int         DependencyCount() const override    { return 1 + sizeof...(TDepValues); }

private:
    using DepHolderT = std::tuple<SharedPtrT<SignalNode<D,TDepValues>>...>;

    SharedPtrT<EventStreamNode<D,E>> events_;
    
    DepHolderT  deps_;
    TFunc       func_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncedIterateByRefNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S,
    typename E,
    typename TFunc,
    typename ... TDepValues
>
class SyncedIterateByRefNode : public SignalNode<D,S>
{
public:
    template <typename T, typename F>
    SyncedIterateByRefNode(T&& init, const std::shared_ptr<EventStreamNode<D,E>>& events, F&& func,
                           const std::shared_ptr<SignalNode<D,TDepValues>>& ... deps) :
        SignalNode{ std::forward<T>(init) },
        events_{ events },
        func_{ std::forward<F>(func) }

    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *events);
        REACT_EXPAND_PACK(Engine::OnNodeAttach(*this, *deps));
    }

    ~SyncedIterateByRefNode()
    {
        Engine::OnNodeDetach(*this, *events_);

        apply(
            DetachFunctor<D,SyncedIterateByRefNode,
                SharedPtrT<SignalNode<D,TDepValues>>...>{ *this },
            deps_);

        Engine::OnNodeDestroy(*this);
    }

    virtual void Tick(void* turnPtr) override
    {
        struct EvalFunctor_
        {
            EvalFunctor_(const E& e, const S& v, TFunc& f) :
                MyEvent{ e },
                MyValue{ v },
                MyFunc{ f }
            {}

            void operator()(const SharedPtrT<SignalNode<D,TDepValues>>& ... args)
            {
                MyFunc(MyEvent, MyValue, args->ValueRef() ...);
            }

            const E&    MyEvent;
            S&          MyValue;
            TFunc&      MyFunc;
        };

        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        for (const auto& e : events_->Events())
            apply(EvalFunctor_{ e, newValue, func_ }, deps_);

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        Engine::OnNodePulse(*this, turn);
    }

    virtual const char* GetNodeType() const override        { return "SyncedIterateByRefNode"; }
    virtual int         DependencyCount() const override    { return 1 + sizeof...(TDepValues); }

private:
    using DepHolderT = std::tuple<SharedPtrT<SignalNode<D,TDepValues>>...>;

    SharedPtrT<EventStreamNode<D,E>> events_;
    
    DepHolderT  deps_;
    TFunc       func_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// HoldNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S
>
class HoldNode : public SignalNode<D,S>
{
public:
    template <typename T>
    HoldNode(T&& init, const SharedPtrT<EventStreamNode<D,S>>& events) :
        SignalNode(std::forward<T>(init)),
        events_{ events }
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *events_);
    }

    ~HoldNode()
    {
        Engine::OnNodeDetach(*this, *events_);
        Engine::OnNodeDestroy(*this);
    }

    virtual const char* GetNodeType() const override    { return "HoldNode"; }

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        bool changed = false;

        if (! events_->Events().empty())
        {
            const S& newValue = events_->Events().back();
            if (newValue != value_)
            {
                changed = true;
                value_ = newValue;
            }
        }

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (changed)
        {
            Engine::OnNodePulse(*this, turn);
        }
        else
        {
            Engine::OnNodeIdlePulse(*this, turn);
        }
    }

    virtual int DependencyCount() const override    { return 1; }

private:
    const SharedPtrT<EventStreamNode<D,S>>    events_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SnapshotNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S,
    typename E
>
class SnapshotNode : public SignalNode<D,S>
{
public:
    SnapshotNode(const SharedPtrT<SignalNode<D,S>>& target,
                 const SharedPtrT<EventStreamNode<D,E>>& trigger) :
        SignalNode{ target->ValueRef() },
        target_{ target },
        trigger_{ trigger }
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *target_);
        Engine::OnNodeAttach(*this, *trigger_);
    }

    ~SnapshotNode()
    {
        Engine::OnNodeDetach(*this, *target_);
        Engine::OnNodeDetach(*this, *trigger_);
        Engine::OnNodeDestroy(*this);
    }

    virtual const char* GetNodeType() const override        { return "SnapshotNode"; }
    virtual int         DependencyCount() const override    { return 2; }

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        trigger_->SetCurrentTurn(turn);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash()));

        bool changed = false;
        
        if (! trigger_->Events().empty())
        {
            const S& newValue = target_->ValueRef();
            if (newValue != value_)
            {
                changed = true;
                value_ = newValue;
            }
        }

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash()));

        if (changed)
        {
            Engine::OnNodePulse(*this, turn);
        }
        else
        {
            Engine::OnNodeIdlePulse(*this, turn);
        }
    }

private:
    const SharedPtrT<SignalNode<D,S>>       target_;
    const SharedPtrT<EventStreamNode<D,E>>  trigger_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MonitorNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename E
>
class MonitorNode : public EventStreamNode<D,E>
{
public:
    MonitorNode(const SharedPtrT<SignalNode<D,E>>& target) :
        EventStreamNode{ },
        target_{ target }
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *target_);
    }

    ~MonitorNode()
    {
        Engine::OnNodeDetach(*this, *target_);
        Engine::OnNodeDestroy(*this);
    }

    virtual const char* GetNodeType() const override        { return "MonitorNode"; }
    virtual int         DependencyCount() const override    { return 1; }

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        SetCurrentTurn(turn, true);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        events_.push_back(target_->ValueRef());

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (events_.size() > 0)
        {
            Engine::OnNodePulse(*this, turn);
        }
        else
        {
            Engine::OnNodeIdlePulse(*this, turn);
        }
    }

private:
    const SharedPtrT<SignalNode<D,E>>    target_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// PulseNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename S,
    typename E
>
class PulseNode : public EventStreamNode<D,S>
{
public:
    PulseNode(const SharedPtrT<SignalNode<D,S>>& target,
              const SharedPtrT<EventStreamNode<D,E>>& trigger) :
        EventStreamNode{ },
        target_{ target },
        trigger_{ trigger }
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *target_);
        Engine::OnNodeAttach(*this, *trigger_);
    }

    ~PulseNode()
    {
        Engine::OnNodeDetach(*this, *target_);
        Engine::OnNodeDetach(*this, *trigger_);
        Engine::OnNodeDestroy(*this);
    }

    virtual const char* GetNodeType() const override        { return "PulseNode"; }
    virtual int         DependencyCount() const override    { return 2; }

    virtual void Tick(void* turnPtr) override
    {
        typedef typename D::Engine::TurnT TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        SetCurrentTurn(turn, true);
        trigger_->SetCurrentTurn(turn);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        for (const auto& e : trigger_->Events())
            events_.push_back(target_->ValueRef());

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (events_.size() > 0)
            Engine::OnNodePulse(*this, turn);
        else
            Engine::OnNodeIdlePulse(*this, turn);
    }

private:
    const SharedPtrT<SignalNode<D,S>>       target_;
    const SharedPtrT<EventStreamNode<D,E>>  trigger_;
};

/****************************************/ REACT_IMPL_END /***************************************/