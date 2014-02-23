#pragma once

#include <atomic>
#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

#include "tbb/spin_mutex.h"

#include "GraphBase.h"
#include "react/common/Types.h"


////////////////////////////////////////////////////////////////////////////////////////
namespace react {

////////////////////////////////////////////////////////////////////////////////////////
/// EventStreamNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename E
>
class EventStreamNode : public ReactiveNode<TDomain,E,void>
{
public:
	typedef std::vector<E>	EventListT;
	typedef tbb::spin_mutex	EventMutexT;

	typedef std::shared_ptr<EventStreamNode> NodePtrT;
	typedef std::weak_ptr<EventStreamNode> NodeWeakPtrT;

	typedef typename TDomain::Engine EngineT;
	typedef typename EngineT::TurnInterface TurnInterface;

	explicit EventStreamNode(bool registered) :
		ReactiveNode(true),
		curTurnId_{ 0 }
	{
		if (!registered)
			registerNode();
	}

	virtual const char* GetNodeType() const override	{ return "EventStreamNode"; }

	EventListT& REvents()
	{
		return events_;
	}

	void SetCurrentTurn(const TurnInterface& turn, bool forceClear = false)
	{
		EventMutexT::scoped_lock lock(eventMutex_);

		if (curTurnId_ != turn.Id() || forceClear)
		{
			curTurnId_ =  turn.Id();
			//printf("Cleared turn\n");
			events_.clear();
		}
	}

	void		ClearREvents()	{ events_.clear(); }

	const E&	Front()			{ events_.front(); }

protected:
	EventListT		events_;
	EventMutexT		eventMutex_;
	uint			curTurnId_;
};

template <typename TDomain, typename E>
using EventStreamNodePtr = typename EventStreamNode<TDomain,E>::NodePtrT;

template <typename TDomain, typename E>
using EventStreamNodeWeakPtr = typename EventStreamNode<TDomain,E>::NodeWeakPtrT;

////////////////////////////////////////////////////////////////////////////////////////
/// EventSourceNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename E
>
class EventSourceNode : public EventStreamNode<TDomain,E>
{
public:
	explicit EventSourceNode(bool registered) :
		EventStreamNode(true)
	{
		admissionFlag_.clear();
		propagationFlag_.clear();


		if (!registered)
			registerNode();
	}

	virtual const char* GetNodeType() const override	{ return "EventSourceNode"; }

	virtual ETickResult Tick(void* turnPtr) override
	{
		ASSERT_(false, "Don't tick EventSourceNode\n");
		return ETickResult::none;
	}

	virtual bool IsInputNode() const override	{ return true; }

	EventSourceNode& Push(const E& e)
	{
		events_.push_back(e);
		return *this;
	}

private:
	std::atomic_flag	admissionFlag_;
	std::atomic_flag	propagationFlag_;

	template <typename>
	friend class TransactionInput;
};

////////////////////////////////////////////////////////////////////////////////////////
/// EventMergeNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename E,
	typename ... TArgs
>
class EventMergeNode : public EventStreamNode<TDomain, E>
{
public:
	typedef typename TDomain::Engine EngineT;
	typedef typename EngineT::TurnInterface TurnInterface;

	EventMergeNode(const EventStreamNodePtr<TDomain, TArgs>& ... args, bool registered) :
		EventStreamNode<TDomain, E>(true),
		deps_{ make_tuple(args ...) },
		func_{ std::bind(&EventMergeNode::expand, this, std::placeholders::_1, args ...) }
	{
		if (!registered)
			registerNode();

		EXPAND_PACK(Engine::OnNodeAttach(*this, *args));
	}

	~EventMergeNode()
	{
		apply
			(
			[this](const EventStreamNodePtr<TDomain, TArgs>& ... args)
		{
			EXPAND_PACK(Engine::OnNodeDetach(*this, *args));
		},
			deps_
			);
	}

	virtual const char* GetNodeType() const override	{ return "EventMergeNode"; }

	virtual ETickResult Tick(void* turnPtr) override
	{

		TurnInterface& turn = *static_cast<TurnInterface*>(turnPtr);

		//printf("EventMergeNode: Tick %08X by thread %08X\n", this, std::this_thread::get_id().hash());

		SetCurrentTurn(turn, true);

		TDomain::Log().template Append<NodeEvaluateBeginEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());
		func_(std::cref(turn));
		TDomain::Log().template Append<NodeEvaluateEndEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());

		if (events_.size() > 0)
		{
			Engine::OnNodePulse(*this, *static_cast<TurnInterface*>(turnPtr));
			return ETickResult::pulsed;
		}
		else
		{
			Engine::OnNodeIdlePulse(*this, *static_cast<TurnInterface*>(turnPtr));
			return ETickResult::idle_pulsed;
		}
	}

	virtual int DependencyCount() const override	{ return sizeof ... (TArgs); }

private:
	std::tuple<EventStreamNodePtr<TDomain, TArgs> ...>	deps_;
	std::function<void(const TurnInterface&)>			func_;

	inline void expand(const TurnInterface& turn, const EventStreamNodePtr<TDomain, TArgs>& ... args)
	{
		EXPAND_PACK(processArgs<TArgs>(turn, args));
	}

	template <typename TArg>
	inline void processArgs(const TurnInterface& turn, const EventStreamNodePtr<TDomain, TArg>& arg)
	{
		arg->SetCurrentTurn(turn);

		events_.insert(events_.end(), arg->REvents().begin(), arg->REvents().end()); // TODO
	}
};

////////////////////////////////////////////////////////////////////////////////////////
/// EventFilterNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename E,
	typename F
>
class EventFilterNode : public EventStreamNode<TDomain, E>
{
public:
	EventFilterNode(const EventStreamNodePtr<TDomain, E>& src, F filter, bool registered) :
		EventStreamNode<TDomain, E>(true),
		src_{ src },
		filter_{ filter }
	{
		if (!registered)
			registerNode();

		Engine::OnNodeAttach(*this, *src_);
	}

	~EventFilterNode()
	{
		Engine::OnNodeDetach(*this, *src_);
	}

	virtual const char* GetNodeType() const override	{ return "EventFilterNode"; }

	virtual ETickResult Tick(void* turnPtr) override
	{
		typedef typename TDomain::Engine EngineT;
		typedef typename EngineT::TurnInterface TurnInterface;
		TurnInterface& turn = *static_cast<TurnInterface*>(turnPtr);

		SetCurrentTurn(turn, true);

		TDomain::Log().template Append<NodeEvaluateBeginEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());
		std::copy_if(src_->REvents().begin(), src_->REvents().end(), std::back_inserter(events_), filter_);
		TDomain::Log().template Append<NodeEvaluateEndEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());

		if (events_.size() > 0)
		{
			Engine::OnNodePulse(*this, *static_cast<TurnInterface*>(turnPtr));
			return ETickResult::pulsed;
		}
		else
		{
			Engine::OnNodeIdlePulse(*this, *static_cast<TurnInterface*>(turnPtr));
			return ETickResult::idle_pulsed;
		}
	}

	virtual int DependencyCount() const override	{ return 1; }

private:
	EventStreamNodePtr<TDomain,E>	src_;

	F	filter_;
};

////////////////////////////////////////////////////////////////////////////////////////
/// EventTransformNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename TIn,
	typename TOut,
	typename F
>
class EventTransformNode : public EventStreamNode<TDomain,TOut>
{
public:
	EventTransformNode(const EventStreamNodePtr<TDomain,TIn>& src, F func, bool registered) :
		EventStreamNode<TDomain,TOut>(true),
		src_{ src },
		func_{ func }
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
		typedef typename TDomain::Engine EngineT;
		typedef typename EngineT::TurnInterface TurnInterface;
		TurnInterface& turn = *static_cast<TurnInterface*>(turnPtr);

		SetCurrentTurn(turn, true);

		TDomain::Log().template Append<NodeEvaluateBeginEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());
		std::transform(src_->REvents().begin(), src_->REvents().end(), std::back_inserter(events_), func_);	
		TDomain::Log().template Append<NodeEvaluateEndEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());

		if (events_.size() > 0)
		{
			Engine::OnNodePulse(*this, *static_cast<TurnInterface*>(turnPtr));
			return ETickResult::pulsed;
		}
		else
		{
			Engine::OnNodeIdlePulse(*this, *static_cast<TurnInterface*>(turnPtr));
			return ETickResult::idle_pulsed;
		}
	}

	virtual int DependencyCount() const override	{ return 1; }

private:
	EventStreamNodePtr<TDomain,TIn>	src_;

	F	func_;
};

// ---
}