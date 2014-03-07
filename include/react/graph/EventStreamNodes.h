#pragma once

#include <atomic>
#include <algorithm>
#include <functional>
#include <memory>
#include <thread>
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
	typename D,
	typename E
>
class EventStreamNode : public ReactiveNode<D,E,void>
{
public:
	typedef std::vector<E>	EventListT;
	typedef tbb::spin_mutex	EventMutexT;

	typedef std::shared_ptr<EventStreamNode> NodePtrT;
	typedef std::weak_ptr<EventStreamNode> NodeWeakPtrT;

	typedef typename D::Engine EngineT;
	typedef typename EngineT::TurnInterface TurnInterface;

	explicit EventStreamNode(bool registered) :
		ReactiveNode(true),
		curTurnId_{ INT_MAX }
	{
		if (!registered)
			registerNode();
	}

	virtual const char* GetNodeType() const override	{ return "EventStreamNode"; }

	EventListT& Events()
	{
		return events_;
	}

	void SetCurrentTurn(const TurnInterface& turn, bool forceUpdate = false, bool noClear = false)
	{// eventMutex_
		EventMutexT::scoped_lock lock(eventMutex_);

		if (curTurnId_ != turn.Id() || forceUpdate)
		{
			curTurnId_ =  turn.Id();
			if (!noClear)
				events_.clear();
		}
	}// ~eventMutex_

	void		ClearEvents()	{ events_.clear(); }

	const E&	Front()			{ events_.front(); }

protected:
	EventListT		events_;
	EventMutexT		eventMutex_;
	uint			curTurnId_;
};

template <typename D, typename E>
using EventStreamNodePtr = typename EventStreamNode<D,E>::NodePtrT;

template <typename D, typename E>
using EventStreamNodeWeakPtr = typename EventStreamNode<D,E>::NodeWeakPtrT;

////////////////////////////////////////////////////////////////////////////////////////
/// EventSourceNode
////////////////////////////////////////////////////////////////////////////////////////
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

	virtual const char* GetNodeType() const override	{ return "EventSourceNode"; }

	virtual ETickResult Tick(void* turnPtr) override
	{
		if (events_.size() > 0 && !changedFlag_)
		{
			typedef typename D::Engine::TurnInterface TurnInterface;
			TurnInterface& turn = *static_cast<TurnInterface*>(turnPtr);

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

	virtual bool IsInputNode() const override	{ return true; }

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

////////////////////////////////////////////////////////////////////////////////////////
/// EventMergeNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename E,
	typename ... TArgs
>
class EventMergeNode : public EventStreamNode<D, E>
{
public:
	typedef typename D::Engine EngineT;
	typedef typename EngineT::TurnInterface TurnInterface;

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

	virtual const char* GetNodeType() const override	{ return "EventMergeNode"; }

	virtual ETickResult Tick(void* turnPtr) override
	{

		TurnInterface& turn = *static_cast<TurnInterface*>(turnPtr);

		//printf("EventMergeNode: Tick %08X by thread %08X\n", this, std::this_thread::get_id().hash());

		SetCurrentTurn(turn);

		D::Log().template Append<NodeEvaluateBeginEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());
		func_(std::cref(turn));
		D::Log().template Append<NodeEvaluateEndEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());

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

	virtual int DependencyCount() const override	{ return sizeof... (TArgs); }

private:
	std::tuple<EventStreamNodePtr<D, TArgs> ...>	deps_;
	std::function<void(const TurnInterface&)>		func_;

	inline void expand(const TurnInterface& turn, const EventStreamNodePtr<D, TArgs>& ... args)
	{
		REACT_EXPAND_PACK(processArgs<TArgs>(turn, args));
	}

	template <typename TArg>
	inline void processArgs(const TurnInterface& turn, const EventStreamNodePtr<D, TArg>& arg)
	{
		arg->SetCurrentTurn(turn);

		events_.insert(events_.end(), arg->Events().begin(), arg->Events().end()); // TODO
	}
};

////////////////////////////////////////////////////////////////////////////////////////
/// EventFilterNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename E,
	typename F
>
class EventFilterNode : public EventStreamNode<D, E>
{
public:
	EventFilterNode(const EventStreamNodePtr<D, E>& src, F filter, bool registered) :
		EventStreamNode<D, E>(true),
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
		typedef typename D::Engine EngineT;
		typedef typename EngineT::TurnInterface TurnInterface;
		TurnInterface& turn = *static_cast<TurnInterface*>(turnPtr);

		SetCurrentTurn(turn, true);

		D::Log().template Append<NodeEvaluateBeginEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());
		std::copy_if(src_->Events().begin(), src_->Events().end(), std::back_inserter(events_), filter_);
		D::Log().template Append<NodeEvaluateEndEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());

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
	EventStreamNodePtr<D,E>	src_;

	F	filter_;
};

////////////////////////////////////////////////////////////////////////////////////////
/// EventTransformNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename TIn,
	typename TOut,
	typename F
>
class EventTransformNode : public EventStreamNode<D,TOut>
{
public:
	EventTransformNode(const EventStreamNodePtr<D,TIn>& src, F func, bool registered) :
		EventStreamNode<D,TOut>(true),
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
		typedef typename D::Engine EngineT;
		typedef typename EngineT::TurnInterface TurnInterface;
		TurnInterface& turn = *static_cast<TurnInterface*>(turnPtr);

		SetCurrentTurn(turn, true);

		D::Log().template Append<NodeEvaluateBeginEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());
		std::transform(src_->Events().begin(), src_->Events().end(), std::back_inserter(events_), func_);	
		D::Log().template Append<NodeEvaluateEndEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());

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
	EventStreamNodePtr<D,TIn>	src_;

	F	func_;
};

// ---
}