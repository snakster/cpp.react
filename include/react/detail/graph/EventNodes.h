
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_GRAPH_EVENTNODES_H_INCLUDED
#define REACT_DETAIL_GRAPH_EVENTNODES_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <atomic>
#include <deque>
#include <memory>
#include <utility>

#include "tbb/spin_mutex.h"

#include "GraphBase.h"
#include "react/common/Concurrency.h"
#include "react/common/Types.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward declarations
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename S>
class SignalNode;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// BufferClearAccessPolicy
///
/// Provides thread safe access to clear event buffer if parallel updating is enabled.
///////////////////////////////////////////////////////////////////////////////////////////////////
// Note: Weird design due to empty base class optimization
template <typename D>
struct BufferClearAccessPolicy :
    private ConditionalCriticalSection<tbb::spin_mutex, D::is_parallel>
{
    template <typename F>
    void AccessBufferForClearing(const F& f) { this->Access(f); }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventStreamNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename E
>
class EventStreamNode :
    public ObservableNode<D>,
    private BufferClearAccessPolicy<D>
{
public:
    using DataT     = std::vector<E>;
    using EngineT   = typename D::Engine;
    using TurnT     = typename EngineT::TurnT;

    EventStreamNode() = default;

    void SetCurrentTurn(const TurnT& turn, bool forceUpdate = false, bool noClear = false)
    {
        this->AccessBufferForClearing([&] {
            if (curTurnId_ != turn.Id() || forceUpdate)
            {
                curTurnId_ =  turn.Id();
                if (!noClear)
                    events_.clear();
            }
        });
    }

    DataT&  Events() { return events_; }

protected:
    DataT   events_;

private:
    uint    curTurnId_  { (std::numeric_limits<uint>::max)() };
};

template <typename D, typename E>
using EventStreamNodePtrT = std::shared_ptr<EventStreamNode<D,E>>;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventSourceNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename E
>
class EventSourceNode :
    public EventStreamNode<D,E>,
    public IInputNode
{
    using Engine = typename EventSourceNode::Engine;

public:
    EventSourceNode() :
        EventSourceNode::EventStreamNode{ }
    {
        Engine::OnNodeCreate(*this);
    }

    ~EventSourceNode()
    {
        Engine::OnNodeDestroy(*this);
    }

    virtual const char* GetNodeType() const override        { return "EventSourceNode"; }
    virtual bool        IsInputNode() const override        { return true; }
    virtual int         DependencyCount() const override    { return 0; }

    virtual void Tick(void* turnPtr) override
    {
        REACT_ASSERT(false, "Ticked EventSourceNode\n");
    }

    template <typename V>
    void AddInput(V&& v)
    {
        // Clear input from previous turn
        if (changedFlag_)
        {
            changedFlag_ = false;
            this->events_.clear();
        }

        this->events_.push_back(std::forward<V>(v));
    }

    virtual bool ApplyInput(void* turnPtr) override
    {
        if (this->events_.size() > 0 && !changedFlag_)
        {
            using TurnT = typename D::Engine::TurnT;
            TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

            this->SetCurrentTurn(turn, true, true);
            changedFlag_ = true;
            Engine::OnInputChange(*this, turn);
            return true;
        }
        else
        {
            return false;
        }
    }

private:
    bool changedFlag_ = false;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventMergeOp
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename E,
    typename ... TDeps
>
class EventMergeOp : public ReactiveOpBase<TDeps...>
{
public:
    template <typename ... TDepsIn>
    EventMergeOp(TDepsIn&& ... deps) :
        EventMergeOp::ReactiveOpBase(DontMove(), std::forward<TDepsIn>(deps) ...)
    {}

    EventMergeOp(EventMergeOp&& other) :
        EventMergeOp::ReactiveOpBase( std::move(other) )
    {}

    template <typename TTurn, typename TCollector>
    void Collect(const TTurn& turn, const TCollector& collector) const
    {
        apply(CollectFunctor<TTurn, TCollector>( turn, collector ), this->deps_);
    }

    template <typename TTurn, typename TCollector, typename TFunctor>
    void CollectRec(const TFunctor& functor) const
    {
        apply(reinterpret_cast<const CollectFunctor<TTurn,TCollector>&>(functor), this->deps_);
    }

private:
    template <typename TTurn, typename TCollector>
    struct CollectFunctor
    {
        CollectFunctor(const TTurn& turn, const TCollector& collector) :
            MyTurn( turn ),
            MyCollector( collector )
        {}

        void operator()(const TDeps& ... deps) const
        {
            REACT_EXPAND_PACK(collect(deps));
        }

        template <typename T>
        void collect(const T& op) const
        {
            op.template CollectRec<TTurn,TCollector>(*this);
        }

        template <typename T>
        void collect(const std::shared_ptr<T>& depPtr) const
        {
            depPtr->SetCurrentTurn(MyTurn);

            for (const auto& v : depPtr->Events())
                MyCollector(v);
        }

        const TTurn&        MyTurn;
        const TCollector&   MyCollector;
    };
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventFilterOp
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename E,
    typename TFilter,
    typename TDep
>
class EventFilterOp : public ReactiveOpBase<TDep>
{
public:
    template <typename TFilterIn, typename TDepIn>
    EventFilterOp(TFilterIn&& filter, TDepIn&& dep) :
        EventFilterOp::ReactiveOpBase{ DontMove(), std::forward<TDepIn>(dep) },
        filter_( std::forward<TFilterIn>(filter) )
    {}

    EventFilterOp(EventFilterOp&& other) :
        EventFilterOp::ReactiveOpBase{ std::move(other) },
        filter_( std::move(other.filter_) )
    {}

    template <typename TTurn, typename TCollector>
    void Collect(const TTurn& turn, const TCollector& collector) const
    {
        collectImpl(turn, FilteredEventCollector<TCollector>{ filter_, collector }, getDep());
    }

    template <typename TTurn, typename TCollector, typename TFunctor>
    void CollectRec(const TFunctor& functor) const
    {
        // Can't recycle functor because MyFunc needs replacing
        Collect<TTurn,TCollector>(functor.MyTurn, functor.MyCollector);
    }

private:
    const TDep& getDep() const { return std::get<0>(this->deps_); }

    template <typename TCollector>
    struct FilteredEventCollector
    {
        FilteredEventCollector(const TFilter& filter, const TCollector& collector) :
            MyFilter( filter ),
            MyCollector( collector )
        {}

        void operator()(const E& e) const
        {
            // Accepted?
            if (MyFilter(e))
                MyCollector(e);
        }

        const TFilter&      MyFilter;
        const TCollector&   MyCollector;    // The wrapped collector
    };

    template <typename TTurn, typename TCollector, typename T>
    static void collectImpl(const TTurn& turn, const TCollector& collector, const T& op)
    {
       op.Collect(turn, collector);
    }

    template <typename TTurn, typename TCollector, typename T>
    static void collectImpl(const TTurn& turn, const TCollector& collector,
                            const std::shared_ptr<T>& depPtr)
    {
        depPtr->SetCurrentTurn(turn);

        for (const auto& v : depPtr->Events())
            collector(v);
    }

    TFilter filter_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventTransformOp
///////////////////////////////////////////////////////////////////////////////////////////////////
// Todo: Refactor code duplication
template
<
    typename E,
    typename TFunc,
    typename TDep
>
class EventTransformOp : public ReactiveOpBase<TDep>
{
public:
    template <typename TFuncIn, typename TDepIn>
    EventTransformOp(TFuncIn&& func, TDepIn&& dep) :
        EventTransformOp::ReactiveOpBase( DontMove(), std::forward<TDepIn>(dep) ),
        func_( std::forward<TFuncIn>(func) )
    {}

    EventTransformOp(EventTransformOp&& other) :
        EventTransformOp::ReactiveOpBase( std::move(other) ),
        func_( std::move(other.func_) )
    {}

    template <typename TTurn, typename TCollector>
    void Collect(const TTurn& turn, const TCollector& collector) const
    {
        collectImpl(turn, TransformEventCollector<TCollector>( func_, collector ), getDep());
    }

    template <typename TTurn, typename TCollector, typename TFunctor>
    void CollectRec(const TFunctor& functor) const
    {
        // Can't recycle functor because MyFunc needs replacing
        Collect<TTurn,TCollector>(functor.MyTurn, functor.MyCollector);
    }

private:
    const TDep& getDep() const { return std::get<0>(this->deps_); }

    template <typename TTarget>
    struct TransformEventCollector
    {
        TransformEventCollector(const TFunc& func, const TTarget& target) :
            MyFunc( func ),
            MyTarget( target )
        {}

        void operator()(const E& e) const
        {
            MyTarget(MyFunc(e));
        }

        const TFunc&    MyFunc;
        const TTarget&  MyTarget;
    };

    template <typename TTurn, typename TCollector, typename T>
    static void collectImpl(const TTurn& turn, const TCollector& collector, const T& op)
    {
       op.Collect(turn, collector);
    }

    template <typename TTurn, typename TCollector, typename T>
    static void collectImpl(const TTurn& turn, const TCollector& collector, const std::shared_ptr<T>& depPtr)
    {
        depPtr->SetCurrentTurn(turn);

        for (const auto& v : depPtr->Events())
            collector(v);
    }

    TFunc func_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventOpNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename E,
    typename TOp
>
class EventOpNode :
    public EventStreamNode<D,E>
{
    using Engine = typename EventOpNode::Engine;

public:
    template <typename ... TArgs>
    EventOpNode(TArgs&& ... args) :
        EventOpNode::EventStreamNode( ),
        op_( std::forward<TArgs>(args) ... )
    {
        Engine::OnNodeCreate(*this);
        op_.template Attach<D>(*this);
    }

    ~EventOpNode()
    {
        if (!wasOpStolen_)
            op_.template Detach<D>(*this);
        Engine::OnNodeDestroy(*this);
    }

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        this->SetCurrentTurn(turn, true);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        {// timer
            size_t count = 0;
            using TimerT = typename EventOpNode::ScopedUpdateTimer;
            TimerT scopedTimer( *this, count );

            op_.Collect(turn, EventCollector( this->events_ ));

            // Note: Count was passed by reference, so we can still change before the dtor
            // of the scoped timer is called
            count = this->events_.size();
        }// ~timer

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (! this->events_.empty())
            Engine::OnNodePulse(*this, turn);
        else
            Engine::OnNodeIdlePulse(*this, turn);
    }

    virtual const char* GetNodeType() const override        { return "EventOpNode"; }
    virtual int         DependencyCount() const override    { return TOp::dependency_count; }

    TOp StealOp()
    {
        REACT_ASSERT(wasOpStolen_ == false, "Op was already stolen.");
        wasOpStolen_ = true;
        op_.template Detach<D>(*this);
        return std::move(op_);
    }

private:
    struct EventCollector
    {
        using DataT = typename EventOpNode::DataT;

        EventCollector(DataT& events) : MyEvents( events )  {}

        void operator()(const E& e) const { MyEvents.push_back(e); }

        DataT& MyEvents;
    };

    TOp     op_;
    bool    wasOpStolen_ = false;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventFlattenNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename TOuter,
    typename TInner
>
class EventFlattenNode : public EventStreamNode<D,TInner>
{
    using Engine = typename EventFlattenNode::Engine;

public:
    EventFlattenNode(const std::shared_ptr<SignalNode<D,TOuter>>& outer,
                     const std::shared_ptr<EventStreamNode<D,TInner>>& inner) :
        EventFlattenNode::EventStreamNode( ),
        outer_( outer ),
        inner_( inner )
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *outer_);
        Engine::OnNodeAttach(*this, *inner_);
    }

    ~EventFlattenNode()
    {
        Engine::OnNodeDetach(*this, *outer_);
        Engine::OnNodeDetach(*this, *inner_);
        Engine::OnNodeDestroy(*this);
    }

    virtual const char* GetNodeType() const override        { return "EventFlattenNode"; }
    virtual bool        IsDynamicNode() const override      { return true; }
    virtual int         DependencyCount() const override    { return 2; }

    virtual void Tick(void* turnPtr) override
    {
        typedef typename D::Engine::TurnT TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        this->SetCurrentTurn(turn, true);
        inner_->SetCurrentTurn(turn);

        auto newInner = GetNodePtr(outer_->ValueRef());

        if (newInner != inner_)
        {
            newInner->SetCurrentTurn(turn);

            // Topology has been changed
            auto oldInner = inner_;
            inner_ = newInner;

            Engine::OnDynamicNodeDetach(*this, *oldInner, turn);
            Engine::OnDynamicNodeAttach(*this, *newInner, turn);

            return;
        }

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        this->events_.insert(
            this->events_.end(),
            inner_->Events().begin(),
            inner_->Events().end());

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (this->events_.size() > 0)
            Engine::OnNodePulse(*this, turn);
        else
            Engine::OnNodeIdlePulse(*this, turn);
    }

private:
    std::shared_ptr<SignalNode<D,TOuter>>       outer_;
    std::shared_ptr<EventStreamNode<D,TInner>>  inner_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncedEventTransformNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename TIn,
    typename TOut,
    typename TFunc,
    typename ... TDepValues
>
class SyncedEventTransformNode :
    public EventStreamNode<D,TOut>
{
    using Engine = typename SyncedEventTransformNode::Engine;

public:
    template <typename F>
    SyncedEventTransformNode(const std::shared_ptr<EventStreamNode<D,TIn>>& source, F&& func, 
                             const std::shared_ptr<SignalNode<D,TDepValues>>& ... deps) :
        SyncedEventTransformNode::EventStreamNode( ),
        source_( source ),
        func_( std::forward<F>(func) ),
        deps_( deps ... )
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *source);
        REACT_EXPAND_PACK(Engine::OnNodeAttach(*this, *deps));
    }

    ~SyncedEventTransformNode()
    {
        Engine::OnNodeDetach(*this, *source_);

        apply(
            DetachFunctor<D,SyncedEventTransformNode,
                std::shared_ptr<SignalNode<D,TDepValues>>...>( *this ),
            deps_);

        Engine::OnNodeDestroy(*this);
    }

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        this->SetCurrentTurn(turn, true);
        // Update of this node could be triggered from deps,
        // so make sure source doesnt contain events from last turn
        source_->SetCurrentTurn(turn);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        // Don't time if there is nothing to do
        if (! source_->Events().empty())
        {// timer
            using TimerT = typename SyncedEventTransformNode::ScopedUpdateTimer;
            TimerT scopedTimer( *this, source_->Events().size() );

            for (const auto& e : source_->Events())
                this->events_.push_back(apply(
                        [this, &e] (const std::shared_ptr<SignalNode<D,TDepValues>>& ... args)
                        {
                            return func_(e, args->ValueRef() ...);
                        },
                        deps_));
                    
        }// ~timer

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (! this->events_.empty())
            Engine::OnNodePulse(*this, turn);
        else
            Engine::OnNodeIdlePulse(*this, turn);
    }

    virtual const char* GetNodeType() const override        { return "SyncedEventTransformNode"; }
    virtual int         DependencyCount() const override    { return 1 + sizeof...(TDepValues); }

private:
    using DepHolderT = std::tuple<std::shared_ptr<SignalNode<D,TDepValues>>...>;

    std::shared_ptr<EventStreamNode<D,TIn>>   source_;

    TFunc       func_;
    DepHolderT  deps_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncedEventFilterNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename E,
    typename TFunc,
    typename ... TDepValues
>
class SyncedEventFilterNode :
    public EventStreamNode<D,E>
{
    using Engine = typename SyncedEventFilterNode::Engine;

public:
    template <typename F>
    SyncedEventFilterNode(const std::shared_ptr<EventStreamNode<D,E>>& source, F&& filter, 
                          const std::shared_ptr<SignalNode<D,TDepValues>>& ... deps) :
        SyncedEventFilterNode::EventStreamNode( ),
        source_( source ),
        filter_( std::forward<F>(filter) ),
        deps_(deps ... )
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *source);
        REACT_EXPAND_PACK(Engine::OnNodeAttach(*this, *deps));
    }

    ~SyncedEventFilterNode()
    {
        Engine::OnNodeDetach(*this, *source_);

        apply(
            DetachFunctor<D,SyncedEventFilterNode,
                std::shared_ptr<SignalNode<D,TDepValues>>...>( *this ),
            deps_);

        Engine::OnNodeDestroy(*this);
    }

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        this->SetCurrentTurn(turn, true);
        // Update of this node could be triggered from deps,
        // so make sure source doesnt contain events from last turn
        source_->SetCurrentTurn(turn);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        // Don't time if there is nothing to do
        if (! source_->Events().empty())
        {// timer
            using TimerT = typename SyncedEventFilterNode::ScopedUpdateTimer;
            TimerT scopedTimer( *this, source_->Events().size() );

            for (const auto& e : source_->Events())
                if (apply(
                        [this, &e] (const std::shared_ptr<SignalNode<D,TDepValues>>& ... args)
                        {
                            return filter_(e, args->ValueRef() ...);
                        },
                        deps_))
                    this->events_.push_back(e);
        }// ~timer

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (! this->events_.empty())
            Engine::OnNodePulse(*this, turn);
        else
            Engine::OnNodeIdlePulse(*this, turn);
    }

    virtual const char* GetNodeType() const override        { return "SyncedEventFilterNode"; }
    virtual int         DependencyCount() const override    { return 1 + sizeof...(TDepValues); }

    virtual bool IsHeavyweight() const override
    {
        return this->IsUpdateThresholdExceeded();
    }

private:
    using DepHolderT = std::tuple<std::shared_ptr<SignalNode<D,TDepValues>>...>;

    std::shared_ptr<EventStreamNode<D,E>>   source_;

    TFunc       filter_;
    DepHolderT  deps_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventProcessingNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename TIn,
    typename TOut,
    typename TFunc
>
class EventProcessingNode :
    public EventStreamNode<D,TOut>
{
    using Engine = typename EventProcessingNode::Engine;

public:
    template <typename F>
    EventProcessingNode(const std::shared_ptr<EventStreamNode<D,TIn>>& source, F&& func) :
        EventProcessingNode::EventStreamNode( ),
        source_( source ),
        func_( std::forward<F>(func) )
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *source);
    }

    ~EventProcessingNode()
    {
        Engine::OnNodeDetach(*this, *source_);
        Engine::OnNodeDestroy(*this);
    }

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        this->SetCurrentTurn(turn, true);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        {// timer
            using TimerT = typename EventProcessingNode::ScopedUpdateTimer;
            TimerT scopedTimer( *this, source_->Events().size() );

            func_(
                EventRange<TIn>( source_->Events() ),
                std::back_inserter(this->events_));

        }// ~timer

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (! this->events_.empty())
            Engine::OnNodePulse(*this, turn);
        else
            Engine::OnNodeIdlePulse(*this, turn);
    }

    virtual const char* GetNodeType() const override        { return "EventProcessingNode"; }
    virtual int         DependencyCount() const override    { return 1; }

private:
    std::shared_ptr<EventStreamNode<D,TIn>>   source_;

    TFunc       func_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncedEventProcessingNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename TIn,
    typename TOut,
    typename TFunc,
    typename ... TDepValues
>
class SyncedEventProcessingNode :
    public EventStreamNode<D,TOut>
{
    using Engine = typename SyncedEventProcessingNode::Engine;

public:
    template <typename F>
    SyncedEventProcessingNode(const std::shared_ptr<EventStreamNode<D,TIn>>& source, F&& func, 
                             const std::shared_ptr<SignalNode<D,TDepValues>>& ... deps) :
        SyncedEventProcessingNode::EventStreamNode( ),
        source_( source ),
        func_( std::forward<F>(func) ),
        deps_( deps ... )
    {
        Engine::OnNodeCreate(*this);
        Engine::OnNodeAttach(*this, *source);
        REACT_EXPAND_PACK(Engine::OnNodeAttach(*this, *deps));
    }

    ~SyncedEventProcessingNode()
    {
        Engine::OnNodeDetach(*this, *source_);

        apply(
            DetachFunctor<D,SyncedEventProcessingNode,
                std::shared_ptr<SignalNode<D,TDepValues>>...>( *this ),
            deps_);

        Engine::OnNodeDestroy(*this);
    }

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        this->SetCurrentTurn(turn, true);
        // Update of this node could be triggered from deps,
        // so make sure source doesnt contain events from last turn
        source_->SetCurrentTurn(turn);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        // Don't time if there is nothing to do
        if (! source_->Events().empty())
        {// timer
            using TimerT = typename SyncedEventProcessingNode::ScopedUpdateTimer;
            TimerT scopedTimer( *this, source_->Events().size() );

            apply(
                [this] (const std::shared_ptr<SignalNode<D,TDepValues>>& ... args)
                {
                    func_(
                        EventRange<TIn>( source_->Events() ),
                        std::back_inserter(this->events_),
                        args->ValueRef() ...);
                },
                deps_);

        }// ~timer

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (! this->events_.empty())
            Engine::OnNodePulse(*this, turn);
        else
            Engine::OnNodeIdlePulse(*this, turn);
    }

    virtual const char* GetNodeType() const override        { return "SycnedEventProcessingNode"; }
    virtual int         DependencyCount() const override    { return 1 + sizeof...(TDepValues); }

private:
    using DepHolderT = std::tuple<std::shared_ptr<SignalNode<D,TDepValues>>...>;

    std::shared_ptr<EventStreamNode<D,TIn>>   source_;

    TFunc       func_;
    DepHolderT  deps_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventJoinNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename ... TValues
>
class EventJoinNode :
    public EventStreamNode<D,std::tuple<TValues ...>>
{
    using Engine = typename EventJoinNode::Engine;
    using TurnT = typename Engine::TurnT;

public:
    EventJoinNode(const std::shared_ptr<EventStreamNode<D,TValues>>& ... sources) :
        EventJoinNode::EventStreamNode( ),
        slots_( sources ... )
    {
        Engine::OnNodeCreate(*this);
        REACT_EXPAND_PACK(Engine::OnNodeAttach(*this, *sources));
    }

    ~EventJoinNode()
    {
        apply(
            [this] (Slot<TValues>& ... slots) {
                REACT_EXPAND_PACK(Engine::OnNodeDetach(*this, *slots.Source));
            },
            slots_);

        Engine::OnNodeDestroy(*this);
    }

    virtual void Tick(void* turnPtr) override
    {
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        this->SetCurrentTurn(turn, true);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        // Don't time if there is nothing to do
        {// timer
            size_t count = 0;
            using TimerT = typename EventJoinNode::ScopedUpdateTimer;
            TimerT scopedTimer( *this, count );

            // Move events into buffers
            apply(
                [this, &turn] (Slot<TValues>& ... slots) {
                    REACT_EXPAND_PACK(fetchBuffer(turn, slots));
                },
                slots_);

            while (true)
            {
                bool isReady = true;

                // All slots ready?
                apply(
                    [this,&isReady] (Slot<TValues>& ... slots) {
                        // Todo: combine return values instead
                        REACT_EXPAND_PACK(checkSlot(slots, isReady));
                    },
                    slots_);

                if (!isReady)
                    break;

                // Pop values from buffers and emit tuple
                apply(
                    [this] (Slot<TValues>& ... slots) {
                        this->events_.emplace_back(slots.Buffer.front() ...);
                        REACT_EXPAND_PACK(slots.Buffer.pop_front());
                    },
                    slots_);
            }

            count = this->events_.size();

        }// ~timer

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (! this->events_.empty())
            Engine::OnNodePulse(*this, turn);
        else
            Engine::OnNodeIdlePulse(*this, turn);
    }

    virtual const char* GetNodeType() const override        { return "EventJoinNode"; }
    virtual int         DependencyCount() const override    { return sizeof...(TValues); }

private:
    template <typename T>
    struct Slot
    {
        Slot(const std::shared_ptr<EventStreamNode<D,T>>& src) :
            Source( src )
        {}

        std::shared_ptr<EventStreamNode<D,T>>   Source;
        std::deque<T>                           Buffer;
    };

    template <typename T>
    static void fetchBuffer(TurnT& turn, Slot<T>& slot)
    {
        slot.Source->SetCurrentTurn(turn);

        slot.Buffer.insert(
            slot.Buffer.end(),
            slot.Source->Events().begin(),
            slot.Source->Events().end());
    }

    template <typename T>
    static void checkSlot(Slot<T>& slot, bool& isReady)
    {
        auto t = isReady && !slot.Buffer.empty();
        isReady = t;
    }

    std::tuple<Slot<TValues>...>    slots_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_GRAPH_EVENTNODES_H_INCLUDED