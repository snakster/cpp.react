#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "EventStreamNodes.h"
#include "SignalNodes.h"
#include "react/logging/EventRecords.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace react {

////////////////////////////////////////////////////////////////////////////////////////
/// FoldBaseNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename S,
	typename E
>
class FoldBaseNode : public SignalNode<D,S>
{
public:
	template <typename T>
	FoldBaseNode(T&& init, const EventStreamNodePtr<D,E>& events, bool registered) :
		SignalNode<D, S>(std::forward<T>(init), true),
		events_{ events }
	{
	}

	virtual ETickResult Tick(void* turnPtr) override
	{
		typedef typename D::Engine::TurnInterface TurnInterface;
		TurnInterface& turn = *static_cast<TurnInterface*>(turnPtr);

		D::Log().template Append<NodeEvaluateBeginEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());
		S newValue = calcNewValue();
		D::Log().template Append<NodeEvaluateEndEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());

		if (! impl::Equals(newValue, value_))
		{
			value_ = newValue;
			Engine::OnNodePulse(*this, turn);
			return ETickResult::pulsed;
		}
		else
		{
			Engine::OnNodeIdlePulse(*this, turn);
			return ETickResult::idle_pulsed;
		}
	}

	virtual int DependencyCount() const	override	{ return 1; }

protected:
	const EventStreamNodePtr<D,E> events_;

	virtual S calcNewValue() const = 0;
};

////////////////////////////////////////////////////////////////////////////////////////
/// FoldNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename S,
	typename E
>
class FoldNode : public FoldBaseNode<D,S,E>
{
public:
	template <typename T, typename F>
	FoldNode(T&& init, const EventStreamNodePtr<D,E>& events, F&& func, bool registered) :
		FoldBaseNode<D,S,E>(std::forward<T>(init), events, true),
		func_{ std::forward<F>(func) }
	{
		if (!registered)
			registerNode();

		Engine::OnNodeAttach(*this, *events);
	}

	~FoldNode()
	{
		Engine::OnNodeDetach(*this, *events_);
	}

	virtual const char* GetNodeType() const override	{ return "FoldNode"; }

protected:
	const std::function<S(S,E)>	func_;

	virtual S calcNewValue() const override
	{
		S newValue = value_;
		for (const auto& e : events_->Events())
			newValue = func_(newValue,e);
			
		return newValue;
	}
};

////////////////////////////////////////////////////////////////////////////////////////
/// IterateNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename S,
	typename E
>
class IterateNode : public FoldBaseNode<D,S,E>
{
public:
	template <typename T, typename F>
	IterateNode(T&& init, const EventStreamNodePtr<D,E>& events, F&& func, bool registered) :
		FoldBaseNode<D,S,E>(std::forward<T>(init), events, true),
		func_{ std::forward<F>(func) }
	{
		if (!registered)
			registerNode();

		Engine::OnNodeAttach(*this, *events);
	}

	~IterateNode()
	{
		Engine::OnNodeDetach(*this, *events_);
	}

	virtual const char* GetNodeType() const override	{ return "IterateNode"; }

protected:
	const std::function<S(S)>	func_;

	virtual S calcNewValue() const override
	{
		S newValue = value_;
		for (const auto& e : events_->Events())
			newValue = func_(newValue);
		return newValue;
	}
};

////////////////////////////////////////////////////////////////////////////////////////
/// HoldNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename S
>
class HoldNode : public SignalNode<D,S>
{
public:
	template <typename T>
	HoldNode(T&& init, const EventStreamNodePtr<D,S>& events, bool registered) :
		SignalNode<D, S>(std::forward<T>(init), true),
		events_{ events }
	{
		if (!registered)
			registerNode();

		Engine::OnNodeAttach(*this, *events_);
	}

	~HoldNode()
	{
		Engine::OnNodeDetach(*this, *events_);
	}

	virtual const char* GetNodeType() const override	{ return "HoldNode"; }

	virtual ETickResult Tick(void* turnPtr) override
	{
		typedef typename D::Engine::TurnInterface TurnInterface;
		TurnInterface& turn = *static_cast<TurnInterface*>(turnPtr);

		D::Log().template Append<NodeEvaluateBeginEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());
		S newValue = value_;
		if (! events_->Events().empty())
			newValue = events_->Events().back();
		D::Log().template Append<NodeEvaluateEndEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());

		if (newValue != value_)
		{
			value_ = newValue;
			Engine::OnNodePulse(*this, turn);
			return ETickResult::pulsed;
		}
		else
		{
			Engine::OnNodeIdlePulse(*this, turn);
			return ETickResult::idle_pulsed;
		}
	}

	virtual int DependencyCount() const override	{ return 1; }

private:
	const EventStreamNodePtr<D,S>	events_;
};

////////////////////////////////////////////////////////////////////////////////////////
/// SnapshotNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename S,
	typename E
>
class SnapshotNode : public SignalNode<D,S>
{
public:
	SnapshotNode(const SignalNodePtr<D,S>& target, const EventStreamNodePtr<D,E>& trigger, bool registered) :
		SignalNode<D, S>(target->Value(), true),
		target_{ target },
		trigger_{ trigger }
	{
		if (!registered)
			registerNode();

		Engine::OnNodeAttach(*this, *target_);
		Engine::OnNodeAttach(*this, *trigger_);
	}

	~SnapshotNode()
	{
		Engine::OnNodeDetach(*this, *target_);
		Engine::OnNodeDetach(*this, *trigger_);
	}

	virtual const char* GetNodeType() const override	{ return "SnapshotNode"; }

	virtual ETickResult Tick(void* turnPtr) override
	{
		typedef typename D::Engine::TurnInterface TurnInterface;
		TurnInterface& turn = *static_cast<TurnInterface*>(turnPtr);

		trigger_->SetCurrentTurn(turn);

		D::Log().template Append<NodeEvaluateBeginEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());
		S newValue = value_;
		if (! trigger_->Events().empty())
			newValue = target_->Value();
		D::Log().template Append<NodeEvaluateEndEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());

		if (newValue != value_)
		{
			value_ = newValue;
			Engine::OnNodePulse(*this, turn);
			return ETickResult::pulsed;
		}
		else
		{
			Engine::OnNodeIdlePulse(*this, turn);
			return ETickResult::idle_pulsed;
		}
	}

	virtual int DependencyCount() const	override	{ return 2; }

private:
	const SignalNodePtr<D,S>		target_;
	const EventStreamNodePtr<D,E>	trigger_;
};

////////////////////////////////////////////////////////////////////////////////////////
/// MonitorNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename E
>
class MonitorNode : public EventStreamNode<D,E>
{
public:
	MonitorNode(const SignalNodePtr<D,E>& target, bool registered) :
		EventStreamNode<D, E>(true),
		target_{ target }
	{
		if (!registered)
			registerNode();

		Engine::OnNodeAttach(*this, *target_);
	}

	~MonitorNode()
	{
		Engine::OnNodeDetach(*this, *target_);
	}

	virtual const char* GetNodeType() const override	{ return "MonitorNode"; }

	virtual ETickResult Tick(void* turnPtr) override
	{
		typedef typename D::Engine::TurnInterface TurnInterface;
		TurnInterface& turn = *static_cast<TurnInterface*>(turnPtr);

		SetCurrentTurn(turn, true);

		D::Log().template Append<NodeEvaluateBeginEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());
		events_.push_back(target_->ValueRef());
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
	const SignalNodePtr<D,E>	target_;
};

////////////////////////////////////////////////////////////////////////////////////////
/// PulseNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename S,
	typename E
>
class PulseNode : public EventStreamNode<D,S>
{
public:
	PulseNode(const SignalNodePtr<D,S>& target, const EventStreamNodePtr<D,E>& trigger, bool registered) :
		EventStreamNode<D, S>(true),
		target_{ target },
		trigger_{ trigger }
	{
		if (!registered)
			registerNode();

		Engine::OnNodeAttach(*this, *target_);
		Engine::OnNodeAttach(*this, *trigger_);
	}

	~PulseNode()
	{
		Engine::OnNodeDetach(*this, *target_);
		Engine::OnNodeDetach(*this, *trigger_);
	}

	virtual const char* GetNodeType() const override	{ return "PulseNode"; }

	virtual ETickResult Tick(void* turnPtr) override
	{
		typedef typename D::Engine::TurnInterface TurnInterface;
		TurnInterface& turn = *static_cast<TurnInterface*>(turnPtr);

		SetCurrentTurn(turn, true);
		trigger_->SetCurrentTurn(turn);

		D::Log().template Append<NodeEvaluateBeginEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());
		for (const auto& e : trigger_->Events())
			events_.push_back(target_->ValueRef());
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

	virtual int DependencyCount() const	{ return 2; }

private:
	const SignalNodePtr<D, S>			target_;
	const EventStreamNodePtr<D, E>	trigger_;
};

////////////////////////////////////////////////////////////////////////////////////////
/// EventFlattenNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename TOuter,
	typename TInner
>
class EventFlattenNode : public EventStreamNode<D, TInner>
{
public:
	EventFlattenNode(const SignalNodePtr<D, TOuter>& outer, const EventStreamNodePtr<D, TInner>& inner, bool registered) :
		EventStreamNode<D, TInner>(true),
		outer_{ outer },
		inner_{ inner }
	{
		if (!registered)
			registerNode();

		Engine::OnNodeAttach(*this, *outer_);
		Engine::OnNodeAttach(*this, *inner_);
	}

	~EventFlattenNode()
	{
		Engine::OnNodeDetach(*this, *outer_);
		Engine::OnNodeDetach(*this, *inner_);
	}

	virtual const char* GetNodeType() const override { return "EventFlattenNode"; }

	virtual bool IsDynamicNode() const override	{ return true; }

	virtual ETickResult Tick(void* turnPtr) override
	{
		typedef typename D::Engine::TurnInterface TurnInterface;
		TurnInterface& turn = *static_cast<TurnInterface*>(turnPtr);

		SetCurrentTurn(turn, true);
		inner_->SetCurrentTurn(turn);

		auto newInner = outer_->Value().GetPtr();

		if (newInner != inner_)
		{
			newInner->SetCurrentTurn(turn);

			// Topology has been changed
			auto oldInner = inner_;
			inner_ = newInner;
			Engine::OnNodeShift(*this, *oldInner, *newInner, turn);
			return ETickResult::invalidated;
		}

		D::Log().template Append<NodeEvaluateBeginEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());
		events_.insert(events_.end(), inner_->Events().begin(), inner_->Events().end());
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

	virtual int DependencyCount() const override	{ return 2; }

private:
	SignalNodePtr<D,TOuter>		outer_;
	EventStreamNodePtr<D,TInner>	inner_;
};

} // ~namespace react