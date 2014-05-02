
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/detail/Defs.h"

#include <atomic>
#include <deque>
#include <memory>

#include "tbb/spin_mutex.h"

#include "GraphBase.h"
#include "react/common/Concurrency.h"
#include "react/common/Types.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

// Empty base class optimization
template <typename D>
struct EventStreamNodeAccessPolicy :
    private ConditionalCriticalSection<
        tbb::spin_mutex,
        EnableNodeUpdateTimer<typename D::Policy::Engine>::value>
{
    template <typename F>
    void AccessCriticalSection(const F& f) { Access(f); }
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
    public ReactiveNode<D,E,void>,
    private EventStreamNodeAccessPolicy<D>
{
public:
    using DataT     = std::deque<E>;
    using EngineT   = typename D::Engine;
    using TurnT     = typename EngineT::TurnT;

    EventStreamNode() = default;

    void SetCurrentTurn(const TurnT& turn, bool forceUpdate = false, bool noClear = false)
    {
        AccessCriticalSection([&] {
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
    uint    curTurnId_ = INT_MAX;
};

template <typename D, typename E>
using EventStreamNodePtrT = SharedPtrT<EventStreamNode<D,E>>;

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
public:
    EventSourceNode() :
        EventStreamNode{ }
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
            events_.clear();
        }

        events_.push_back(std::forward<V>(v));
    }

    virtual bool ApplyInput(void* turnPtr) override
    {
        if (events_.size() > 0 && !changedFlag_)
        {
            using TurnT = typename D::Engine::TurnT;
            TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

            SetCurrentTurn(turn, true, true);
            changedFlag_ = true;
            Engine::OnTurnInputChange(*this, turn);
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
        ReactiveOpBase(0u, std::forward<TDepsIn>(deps) ...)
    {}

    EventMergeOp(EventMergeOp&& other) :
        ReactiveOpBase{ std::move(other) }
    {}

    template <typename TTurn, typename TCollector>
    void Collect(const TTurn& turn, const TCollector& collector) const
    {
        apply(CollectFunctor<TTurn, TCollector>{ turn, collector }, deps_);
    }

    template <typename TTurn, typename TCollector, typename TFunctor>
    void CollectRec(const TFunctor& functor) const
    {
        apply(reinterpret_cast<const CollectFunctor<TTurn,TCollector>&>(functor), deps_);
    }

private:
    template <typename TTurn, typename TCollector>
    struct CollectFunctor
    {
        CollectFunctor(const TTurn& turn, const TCollector& collector) :
            MyTurn{ turn },
            MyCollector{ collector }
        {}

        void operator()(const TDeps& ... deps) const
        {
            REACT_EXPAND_PACK(collect(deps));
        }

        template <typename T>
        void collect(const T& op) const
        {
            op.CollectRec<TTurn,TCollector>(*this);
        }

        template <typename T>
        void collect(const SharedPtrT<T>& depPtr) const
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
        ReactiveOpBase{ 0u, std::forward<TDepIn>(dep) },
        filter_{ std::forward<TFilterIn>(filter) }
    {}

    EventFilterOp(EventFilterOp&& other) :
        ReactiveOpBase{ std::move(other) },
        filter_{ std::move(other.filter_) }
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
    const TDep& getDep() const { return std::get<0>(deps_); }

    template <typename TFiltered>
    struct FilteredEventCollector
    {
        FilteredEventCollector(const TFilter& filter, const TFiltered& filtered) :
            MyFilter{ filter },
            MyFiltered{ filtered }
        {}

        void operator()(const E& e) const
        {
            // Accepted?
            if (MyFilter(e))
                MyFiltered(e);
        }

        const TFilter&      MyFilter;
        const TFiltered&    MyFiltered;
    };

    template <typename TTurn, typename TCollector, typename T>
    static void collectImpl(const TTurn& turn, const TCollector& collector, const T& op)
    {
       op.Collect(turn, collector);
    }

    template <typename TTurn, typename TCollector, typename T>
    static void collectImpl(const TTurn& turn, const TCollector& collector, const SharedPtrT<T>& depPtr)
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
        ReactiveOpBase{ 0u, std::forward<TDepIn>(dep) },
        func_{ std::forward<TFuncIn>(func) }
    {}

    EventTransformOp(EventTransformOp&& other) :
        ReactiveOpBase{ std::move(other) },
        func_{ std::move(other.func_) }
    {}

    template <typename TTurn, typename TCollector>
    void Collect(const TTurn& turn, const TCollector& collector) const
    {
        collectImpl(turn, TransformEventCollector<TCollector>{ func_, collector }, getDep());
    }

    template <typename TTurn, typename TCollector, typename TFunctor>
    void CollectRec(const TFunctor& functor) const
    {
        // Can't recycle functor because MyFunc needs replacing
        Collect<TTurn,TCollector>(functor.MyTurn, functor.MyCollector);
    }

private:
    const TDep& getDep() const { return std::get<0>(deps_); }

    template <typename TTarget>
    struct TransformEventCollector
    {
        TransformEventCollector(const TFunc& func, const TTarget& target) :
            MyFunc{ func },
            MyTarget{ target }
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
    static void collectImpl(const TTurn& turn, const TCollector& collector, const SharedPtrT<T>& depPtr)
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
class EventOpNode : public EventStreamNode<D,E>
{
public:
    template <typename ... TArgs>
    EventOpNode(TArgs&& ... args) :
        EventStreamNode{ },
        op_{ std::forward<TArgs>(args) ... }
    {
        Engine::OnNodeCreate(*this);
        op_.Attach<D>(*this);
    }

    ~EventOpNode()
    {
        if (!wasOpStolen_)
            op_.Detach<D>(*this);
        Engine::OnNodeDestroy(*this);
    }

    virtual const char* GetNodeType() const override        { return "EventOpNode"; }
    virtual int         DependencyCount() const override    { return TOp::dependency_count; }

    virtual void Tick(void* turnPtr) override
    {
        using TurnT = typename D::Engine::TurnT;
        TurnT& turn = *reinterpret_cast<TurnT*>(turnPtr);

        SetCurrentTurn(turn, true);

        REACT_LOG(D::Log().template Append<NodeEvaluateBeginEvent>(
            GetObjectId(*this), turn.Id()));

        op_.Collect(turn, EventCollector{ Events() });

        REACT_LOG(D::Log().template Append<NodeEvaluateEndEvent>(
            GetObjectId(*this), turn.Id()));

        if (! events_.empty())
        {
            Engine::OnNodePulse(*this, turn);
        }
        else
        {
            Engine::OnNodeIdlePulse(*this, turn);
        }
    }

    TOp StealOp()
    {
        REACT_ASSERT(wasOpStolen_ == false, "Op was already stolen.");
        wasOpStolen_ = true;
        op_.Detach<D>(*this);
        return std::move(op_);
    }

private:
    struct EventCollector
    {
        EventCollector(DataT& events) : MyEvents{ events }  {}

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
    typename S
>
class SignalNode;

template
<
    typename D,
    typename TOuter,
    typename TInner
>
class EventFlattenNode : public EventStreamNode<D,TInner>
{
public:
    EventFlattenNode(const SharedPtrT<SignalNode<D,TOuter>>& outer,
                     const SharedPtrT<EventStreamNode<D,TInner>>& inner) :
        EventStreamNode{ },
        outer_{ outer },
        inner_{ inner }
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

        SetCurrentTurn(turn, true);
        inner_->SetCurrentTurn(turn);

        auto newInner = outer_->ValueRef().NodePtr();

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

        events_.insert(events_.end(), inner_->Events().begin(), inner_->Events().end());

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
    SharedPtrT<SignalNode<D,TOuter>>        outer_;
    SharedPtrT<EventStreamNode<D,TInner>>   inner_;
};

/****************************************/ REACT_IMPL_END /***************************************/
