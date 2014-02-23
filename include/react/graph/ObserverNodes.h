#pragma once

#include <memory>
#include <functional>

#include "GraphBase.h"

namespace react {

////////////////////////////////////////////////////////////////////////////////////////
/// IObserverNode
////////////////////////////////////////////////////////////////////////////////////////
struct IObserverNode
{
	virtual ~IObserverNode() {}

	virtual void Detach() = 0;
};

namespace current_observer_state_
{
	static __declspec(thread) bool	shouldDetach = false;
}

////////////////////////////////////////////////////////////////////////////////////////
/// ObserverNode
////////////////////////////////////////////////////////////////////////////////////////
template <typename TDomain>
class ObserverNode :
	public ReactiveNode<TDomain,void,void>,
	public IObserverNode
{
public:
	typedef std::shared_ptr<ObserverNode> NodePtr;

	explicit ObserverNode(bool registered) :
		ReactiveNode<TDomain,void,void>(true)
	{
	}

	virtual const char*	GetNodeType() const		{ return "ObserverNode"; }
	virtual bool		IsOutputNode() const	{ return true; }
};

template <typename TDomain>
using ObserverNodePtr = typename ObserverNode<TDomain>::NodePtr;

////////////////////////////////////////////////////////////////////////////////////////
/// SignalObserverNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename TArg
>
class SignalObserverNode : public ObserverNode<TDomain>
{
public:
	template <typename F>
	SignalObserverNode(const SignalNodePtr<TDomain,TArg>& subject, const F& func, bool registered) :
		ObserverNode<TDomain>(true),
		subject_{ subject },
		func_{ func }
	{
		if (!registered)
			registerNode();

		subject->IncObsCount();

		Engine::OnNodeAttach(*this, *subject);
	}

	virtual const char* GetNodeType() const override { return "SignalObserverNode"; }

	virtual ETickResult Tick(void* turnPtr) override
	{
		typedef typename TDomain::Engine::TurnInterface TurnInterface;
		TurnInterface& turn = *static_cast<TurnInterface*>(turnPtr);

		TDomain::Log().template Append<NodeEvaluateBeginEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());

		current_observer_state_::shouldDetach = false;

		TDomain::TransactionInputContinuation::Set(&turn.InputContinuation());

		if (auto p = subject_.lock())
			func_(p->ValueRef());

		TDomain::TransactionInputContinuation::Reset();

		if (current_observer_state_::shouldDetach)
			turn.QueueForDetach(*this);

		TDomain::Log().template Append<NodeEvaluateEndEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());
		return ETickResult::none;
	}

	virtual int DependencyCount() const	{ return 1; }

	virtual void Detach()
	{
		if (auto p = subject_.lock())
		{
			p->DecObsCount();

			Engine::OnNodeDetach(*this, *p);
			subject_.reset();
		}
	}

private:
	SignalNodeWeakPtr<TDomain,TArg>		subject_;
	std::function<void(TArg)>			func_;
};

////////////////////////////////////////////////////////////////////////////////////////
/// EventObserverNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename TArg
>
class EventObserverNode : public ObserverNode<TDomain>
{
public:
	template <typename F>
	EventObserverNode(const EventStreamNodePtr<TDomain,TArg>& subject, const F& func, bool registered) :
		ObserverNode<TDomain>(true),
		subject_{ subject },
		func_{ func }
	{
		if (!registered)
			registerNode();

		subject->IncObsCount();

		Engine::OnNodeAttach(*this, *subject);
	}

	virtual const char* GetNodeType() const override { return "EventObserverNode"; }

	virtual ETickResult Tick(void* turnPtr) override
	{
		typedef typename TDomain::Engine::TurnInterface TurnInterface;
		TurnInterface& turn = *static_cast<TurnInterface*>(turnPtr);

		TDomain::Log().template Append<NodeEvaluateBeginEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());
		
		current_observer_state_::shouldDetach = false;

		TDomain::TransactionInputContinuation::Set(&turn.InputContinuation());

		if (auto p = subject_.lock())
		{
			for (const auto& e : p->Events())
				func_(e);
		}

		TDomain::TransactionInputContinuation::Reset();

		if (current_observer_state_::shouldDetach)
			turn.QueueForDetach(*this);

		TDomain::Log().template Append<NodeEvaluateEndEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());

		return ETickResult::none;
	}

	virtual int DependencyCount() const	{ return 1; }

	virtual void Detach()
	{
		if (auto p = subject_.lock())
		{
			p->DecObsCount();

			Engine::OnNodeDetach(*this, *p);
			subject_.reset();
		}
	}

private:
	EventStreamNodeWeakPtr<TDomain,TArg>	subject_;
	std::function<void(TArg)>				func_;
};

// ---
}