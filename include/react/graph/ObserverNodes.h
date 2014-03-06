#pragma once

#include <memory>
#include <functional>

#include "GraphBase.h"
#include "EventStreamNodes.h"
#include "SignalNodes.h"

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
template <typename D>
class ObserverNode :
	public ReactiveNode<D,void,void>,
	public IObserverNode
{
public:
	typedef std::shared_ptr<ObserverNode> NodePtr;

	explicit ObserverNode(bool registered) :
		ReactiveNode<D,void,void>(true)
	{
	}

	virtual const char*	GetNodeType() const		{ return "ObserverNode"; }
	virtual bool		IsOutputNode() const	{ return true; }
};

template <typename D>
using ObserverNodePtr = typename ObserverNode<D>::NodePtr;

////////////////////////////////////////////////////////////////////////////////////////
/// SignalObserverNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename TArg
>
class SignalObserverNode : public ObserverNode<D>
{
public:
	template <typename F>
	SignalObserverNode(const SignalNodePtr<D,TArg>& subject, const F& func, bool registered) :
		ObserverNode<D>(true),
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
		typedef typename D::Engine::TurnInterface TurnInterface;
		TurnInterface& turn = *static_cast<TurnInterface*>(turnPtr);

		D::Log().template Append<NodeEvaluateBeginEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());

		current_observer_state_::shouldDetach = false;

		//ContinuationInputHolder_::Set(&turn.Continuation());
		D::SetCurrentContinuation(turn.Continuation());

		if (auto p = subject_.lock())
			func_(p->ValueRef());

		D::ClearCurrentContinuation();

		//D::TransactionInputContinuation::Reset();

		if (current_observer_state_::shouldDetach)
			turn.QueueForDetach(*this);

		D::Log().template Append<NodeEvaluateEndEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());
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
	SignalNodeWeakPtr<D,TArg>		subject_;
	std::function<void(TArg)>			func_;
};

////////////////////////////////////////////////////////////////////////////////////////
/// EventObserverNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename TArg
>
class EventObserverNode : public ObserverNode<D>
{
public:
	template <typename F>
	EventObserverNode(const EventStreamNodePtr<D,TArg>& subject, const F& func, bool registered) :
		ObserverNode<D>(true),
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
		typedef typename D::Engine::TurnInterface TurnInterface;
		TurnInterface& turn = *static_cast<TurnInterface*>(turnPtr);

		D::Log().template Append<NodeEvaluateBeginEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());
		
		current_observer_state_::shouldDetach = false;

		D::TransactionInputContinuation::Set(&turn.InputContinuation());

		if (auto p = subject_.lock())
		{
			for (const auto& e : p->Events())
				func_(e);
		}

		D::TransactionInputContinuation::Reset();

		if (current_observer_state_::shouldDetach)
			turn.QueueForDetach(*this);

		D::Log().template Append<NodeEvaluateEndEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());

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
	EventStreamNodeWeakPtr<D,TArg>	subject_;
	std::function<void(TArg)>				func_;
};

// ---
}