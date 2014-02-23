#pragma once

#include <algorithm>
#include <functional>
#include <memory>
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
	typename TDomain,
	typename S,
	typename E,
	typename ... TArgs
>
class FoldBaseNode : public SignalNode<TDomain,S>
{
public:
	FoldBaseNode(const S& initialValue, const EventStreamNodePtr<TDomain,E>& events,
			 const SignalNodePtr<TDomain,TArgs>& ... args, bool registered) :
		SignalNode<TDomain, S>(initialValue, true),
		deps_{ std::make_tuple(args ...) },
		events_{ events }
	{
	}

	virtual const char* GetNodeType() const override	{ return "FoldBaseNode"; }

	virtual ETickResult Tick(void* turnPtr) override
	{
		typedef typename TDomain::Engine::TurnInterface TurnInterface;
		TurnInterface& turn = *static_cast<TurnInterface*>(turnPtr);

		TDomain::Log().template Append<NodeEvaluateBeginEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());
		S newValue = calcNewValue();
		TDomain::Log().template Append<NodeEvaluateEndEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());

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

	virtual int DependencyCount() const	override	{ return 1 + sizeof ... (TArgs); }

protected:
	const EventStreamNodePtr<TDomain,E>					events_;
	const std::tuple<SignalNodePtr<TDomain,TArgs> ...>	deps_;

	virtual S calcNewValue() const = 0;

	template <typename A>
	S evaluate(const A& a) const
	{
		return apply(a, apply(unpackValues, deps_));
	}

	static inline auto unpackValues(const SignalNodePtr<TDomain,TArgs>& ... args) -> std::tuple<TArgs ...>
	{
		return std::make_tuple(args->ValueRef() ...);
	}
};

////////////////////////////////////////////////////////////////////////////////////////
/// FoldNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename S,
	typename E,
	typename ... TArgs
>
class FoldNode : public FoldBaseNode<TDomain,S,E,TArgs...>
{
public:
	FoldNode(const S& initialValue, const EventStreamNodePtr<TDomain,E>& events,
			 const SignalNodePtr<TDomain,TArgs>& ... args, std::function<S(S,E,TArgs ...)> func, bool registered) :
		FoldBaseNode<TDomain, S, E, TArgs...>(initialValue, events, args ..., true),
		func_{ func }
	{
		if (!registered)
			registerNode();

		Engine::OnNodeAttach(*this, *events);
		EXPAND_PACK(Engine::OnNodeAttach(*this, *args));
	}

	~FoldNode()
	{
		Engine::OnNodeDetach(*this, *events_);

		apply
		(
			[this] (const SignalNodePtr<TDomain,TArgs>& ... args)
			{
				EXPAND_PACK(Engine::OnNodeDetach(*this, *args));
			},
			deps_
		);
	}

	virtual const char* GetNodeType() const override	{ return "FoldNode"; }

protected:
	const std::function<S(S,E,TArgs ...)>	func_;

	struct ApplyHelper_
	{
		ApplyHelper_(const S& s, const E& e, const std::function<S(S,E,TArgs ...)> func) :
			s_{ s }, e_{ e }, func_{ func }
		{
		}

		S operator() (TArgs ... args) const
		{
			return func_(s_, e_, args ...);
		}

		const S& s_;
		const E& e_;
		const std::function<S(S,E,TArgs ...)> func_;
	};

	virtual S calcNewValue() const override
	{
		S newValue = value_;
		for (auto e : events_->REvents())
			newValue = evaluate(ApplyHelper_(newValue,e,func_));
		return newValue;
	}
};

////////////////////////////////////////////////////////////////////////////////////////
/// IterateNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename S,
	typename E,
	typename ... TArgs
>
class IterateNode : public FoldBaseNode<TDomain,S,E,TArgs...>
{
public:
	IterateNode(const S& initialValue, const EventStreamNodePtr<TDomain,E>& events,
			 const SignalNodePtr<TDomain,TArgs>& ... args, std::function<S(S,TArgs ...)> func, bool registered) :
		FoldBaseNode<TDomain, S, E, TArgs...>(initialValue, events, args ..., true),
		 func_{ func }
	{
		if (!registered)
			registerNode();

		Engine::OnNodeAttach(*this, *events);
		EXPAND_PACK(Engine::OnNodeAttach(*this, *args));
	}

	~IterateNode()
	{
		Engine::OnNodeDetach(*this, *events_);

		apply
		(
			[this] (const SignalNodePtr<TDomain,TArgs>& ... args)
			{
				EXPAND_PACK(Engine::OnNodeDetach(*this, *args));
			},
			deps_
		);
	}

	virtual const char* GetNodeType() const override	{ return "IterateNode"; }

protected:
	const std::function<S(S,TArgs ...)>	func_;

	struct ApplyHelper_
	{
		ApplyHelper_(const S& s, const std::function<S(S,TArgs ...)> func) :
			s_{ s }, func_{ func }
		{
		}

		S operator() (TArgs ... args) const
		{
			return func_(s_, args ...);
		}

		const S& s_;
		const std::function<S(S,TArgs ...)> func_;
	};

	virtual S calcNewValue() const override
	{
		S newValue = value_;
		for (int i=0; i < events_->REvents().size(); i++)
			newValue = evaluate(ApplyHelper_(newValue,func_));
		return newValue;
	}
};

////////////////////////////////////////////////////////////////////////////////////////
/// HoldNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename S
>
class HoldNode : public SignalNode<TDomain,S>
{
public:
	HoldNode(const S initialValue, const EventStreamNodePtr<TDomain,S>& events, bool registered) :
		SignalNode<TDomain, S>(initialValue, true),
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
		typedef typename TDomain::Engine::TurnInterface TurnInterface;
		TurnInterface& turn = *static_cast<TurnInterface*>(turnPtr);

		TDomain::Log().template Append<NodeEvaluateBeginEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());
		S newValue = value_;
		if (! events_->REvents().empty())
			newValue = events_->REvents().back();
		TDomain::Log().template Append<NodeEvaluateEndEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());

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
	const EventStreamNodePtr<TDomain,S>	events_;
};

////////////////////////////////////////////////////////////////////////////////////////
/// SnapshotNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename S,
	typename E
>
class SnapshotNode : public SignalNode<TDomain,S>
{
public:
	SnapshotNode(const SignalNodePtr<TDomain,S>& target, const EventStreamNodePtr<TDomain,E>& trigger, bool registered) :
		SignalNode<TDomain, S>(target->Value(), true),
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
		typedef typename TDomain::Engine::TurnInterface TurnInterface;
		TurnInterface& turn = *static_cast<TurnInterface*>(turnPtr);

		trigger_->SetCurrentTurn(turn);

		TDomain::Log().template Append<NodeEvaluateBeginEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());
		S newValue = value_;
		if (! trigger_->REvents().empty())
			newValue = target_->Value();
		TDomain::Log().template Append<NodeEvaluateEndEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());

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
	const SignalNodePtr<TDomain,S>		target_;
	const EventStreamNodePtr<TDomain,E>	trigger_;
};

////////////////////////////////////////////////////////////////////////////////////////
/// MonitorNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename E
>
class MonitorNode : public EventStreamNode<TDomain,E>
{
public:
	MonitorNode(const SignalNodePtr<TDomain,E>& target, bool registered) :
		EventStreamNode<TDomain, E>(true),
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
		typedef typename TDomain::Engine::TurnInterface TurnInterface;
		TurnInterface& turn = *static_cast<TurnInterface*>(turnPtr);

		SetCurrentTurn(turn, true);

		TDomain::Log().template Append<NodeEvaluateBeginEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());
		events_.push_back(target_->ValueRef());
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
	const SignalNodePtr<TDomain,E>	target_;
};

////////////////////////////////////////////////////////////////////////////////////////
/// PulseNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename S,
	typename E
>
class PulseNode : public EventStreamNode<TDomain,S>
{
public:
	PulseNode(const SignalNodePtr<TDomain,S>& target, const EventStreamNodePtr<TDomain,E>& trigger, bool registered) :
		EventStreamNode<TDomain, S>(true),
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
		typedef typename TDomain::Engine::TurnInterface TurnInterface;
		TurnInterface& turn = *static_cast<TurnInterface*>(turnPtr);

		SetCurrentTurn(turn, true);
		trigger_->SetCurrentTurn(turn);

		TDomain::Log().template Append<NodeEvaluateBeginEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());
		for (int i=0; i < trigger_->REvents().size(); i++)
			events_.push_back(target_->ValueRef());
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

	virtual int DependencyCount() const	{ return 2; }

private:
	const SignalNodePtr<TDomain, S>			target_;
	const EventStreamNodePtr<TDomain, E>	trigger_;
};

////////////////////////////////////////////////////////////////////////////////////////
/// EventFlattenNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename TOuter,
	typename TInner
>
class EventFlattenNode : public EventStreamNode<TDomain, TInner>
{
public:
	EventFlattenNode(const SignalNodePtr<TDomain, TOuter>& outer, const EventStreamNodePtr<TDomain, TInner>& inner, bool registered) :
		EventStreamNode<TDomain, TInner>(true),
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
		typedef typename TDomain::Engine::TurnInterface TurnInterface;
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

		TDomain::Log().template Append<NodeEvaluateBeginEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());
		events_.insert(events_.end(), inner_->REvents().begin(), inner_->REvents().end());
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

	virtual int DependencyCount() const override	{ return 2; }

private:
	SignalNodePtr<TDomain,TOuter>		outer_;
	EventStreamNodePtr<TDomain,TInner>	inner_;
};

} // ~namespace react