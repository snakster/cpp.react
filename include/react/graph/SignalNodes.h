#pragma once

#include <memory>
#include <functional>
#include <tuple>

#include "GraphBase.h"
#include "react/Signal.h"
#include "react/EventStream.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace react {

template <typename T>
class Reactive;

template <typename TDomain, typename S>
class Signal_;

template <typename TDomain, typename E>
class REvents;

////////////////////////////////////////////////////////////////////////////////////////
/// SignalNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename S
>
class SignalNode : public ReactiveNode<TDomain,S,S>
{
public:
	typedef std::shared_ptr<SignalNode> NodePtrT;
	typedef std::weak_ptr<SignalNode>	NodeWeakPtrT;

	explicit SignalNode(bool registered) :
		ReactiveNode{ true }
	{
		if (!registered)
			registerNode();
	}

	SignalNode(S value, bool registered) :
		ReactiveNode(true),
		value_(value)
	{
		if (!registered)
			registerNode();
	}

	virtual const char* GetNodeType() const override { return "SignalNode"; }

	virtual ETickResult Tick(void* turnPtr) override
	{
		ASSERT_(false, "Don't tick SignalNode\n");
		return ETickResult::none;
	}

	S Value() const
	{
		return value_;
	}

	S operator()(void) const
	{
		return value_;
	}

	const S& ValueRef() const
	{
		return value_;
	}

protected:
	S		value_;
};

template <typename TDomain, typename S>
using SignalNodePtr = typename SignalNode<TDomain,S>::NodePtrT;

template <typename TDomain, typename S>
using SignalNodeWeakPtr = typename SignalNode<TDomain,S>::NodeWeakPtrT;

////////////////////////////////////////////////////////////////////////////////////////
/// VarNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename S
>
class VarNode : public SignalNode<TDomain,S>
{
public:
	VarNode(S value, bool registered) :
		SignalNode<TDomain,S>(value, true)
	{
		if (!registered)
			registerNode();
	}

	virtual const char* GetNodeType() const override { return "VarNode"; }

	virtual ETickResult Tick(void* turnPtr) override
	{
		ASSERT_(false, "Don't tick VarNode\n");
		return ETickResult::none;
	}

	virtual bool IsInputNode() const override	{ return true; }

	void SetNewValue(const S& newValue)
	{
		newValue_ = newValue;
	}

	bool ApplyNewValue()
	{
		if (! equals(value_, newValue_))
		{
			value_ = newValue_;
			return true;
		}
		else
		{
			return false;
		}
	}

private:
	S		newValue_;

	template <typename L, typename R>
	static bool equals(L lhs, R rhs)
	{
		return lhs == rhs;
	}

	// todo - ugly
	template <typename L, typename R>
	static bool equals(const REvents<TDomain,L>& lhs, const REvents<TDomain,R>& rhs)
	{
		return lhs.Equals(rhs);
	}

	template <typename L, typename R>
	static bool equals(const Signal_<TDomain,L>& lhs, const Signal_<TDomain,R>& rhs)
	{
		return lhs.Equals(rhs);
	}
};

////////////////////////////////////////////////////////////////////////////////////////
/// ValNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename S
>
class ValNode : public SignalNode<TDomain,S>
{
public:
	ValNode(const S value, bool registered) :
		SignalNode<TDomain,S>(value, true)
	{
		if (!registered)
			registerNode();
	}

	virtual const char* GetNodeType() const override { return "ValNode"; }

	virtual ETickResult Tick(void* turnPtr) override
	{
		ASSERT_(false, "Don't tick the ValNode\n");
		return ETickResult::none;
	}
};

////////////////////////////////////////////////////////////////////////////////////////
/// FunctionNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename S,
	typename ... TArgs
>
class FunctionNode : public SignalNode<TDomain,S>
{
public:
	FunctionNode(const SignalNodePtr<TDomain,TArgs>& ... args, std::function<S(TArgs ...)> func, bool registered) :
		SignalNode<TDomain, S>(true),
		deps_{ make_tuple(args ...) },
		func_{ func }
	{
		if (!registered)
			registerNode();

		value_ = func_(args->Value() ...);

		EXPAND_PACK(Engine::OnNodeAttach(*this, *args));
	}

	~FunctionNode()
	{
		apply
		(
			[this] (const SignalNodePtr<TDomain,TArgs>& ... args)
			{
				EXPAND_PACK(Engine::OnNodeDetach(*this, *args));
			},
			deps_
		);
	}

	virtual const char* GetNodeType() const override { return "FunctionNode"; }

	virtual ETickResult Tick(void* turnPtr) override
	{
		typedef typename TDomain::Engine::TurnInterface TurnInterface;
		TurnInterface& turn = *static_cast<TurnInterface*>(turnPtr);

		TDomain::Log().template Append<NodeEvaluateBeginEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());
		S newValue = evaluate();
		TDomain::Log().template Append<NodeEvaluateEndEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());

		
		//if (newValue != value_)
		if (! equals(value_, newValue))
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

	virtual int DependencyCount() const override	{ return sizeof ... (TArgs); }

private:
	std::tuple<SignalNodePtr<TDomain,TArgs> ...>	deps_;
	std::function<S(TArgs ...)>						func_;

	S evaluate() const
	{
		return apply(func_, apply(unpackValues, deps_));
	}

	static inline auto unpackValues(const SignalNodePtr<TDomain,TArgs>& ... args) -> std::tuple<TArgs ...>
	{
		return std::make_tuple(args->ValueRef() ...);
	}

	template <typename L, typename R>
	static bool equals(L lhs, R rhs)
	{
		return lhs == rhs;
	}

	// todo - ugly
	template <typename L, typename R>
	static bool equals(const REvents<TDomain,L>& lhs, const REvents<TDomain,R>& rhs)
	{
		return lhs.Equals(rhs);
	}

	template <typename L, typename R>
	static bool equals(const Signal_<TDomain,L>& lhs, const Signal_<TDomain,R>& rhs)
	{
		return lhs.Equals(rhs);
	}
};

////////////////////////////////////////////////////////////////////////////////////////
/// FlattenNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename TOuter,
	typename TInner
>
class FlattenNode : public SignalNode<TDomain,TInner>
{
public:
	FlattenNode(const SignalNodePtr<TDomain,TOuter>& outer, const SignalNodePtr<TDomain,TInner>& inner, bool registered) :
		SignalNode<TDomain, TInner>(inner->Value(), true),
		outer_{ outer },
		inner_{ inner }
	{
		if (!registered)
			registerNode();

		Engine::OnNodeAttach(*this, *outer_);
		Engine::OnNodeAttach(*this, *inner_);

		
	}

	~FlattenNode()
	{
		Engine::OnNodeDetach(*this, *inner_);
		Engine::OnNodeDetach(*this, *outer_);
	}

	virtual const char* GetNodeType() const override { return "FlattenNode"; }

	virtual bool IsDynamicNode() const override	{ return true; }

	virtual ETickResult Tick(void* turnPtr) override
	{
		typedef typename TDomain::Engine::TurnInterface TurnInterface;
		TurnInterface& turn = *static_cast<TurnInterface*>(turnPtr);

		auto newInner = outer_->Value().GetPtr();

		if (newInner != inner_)
		{
			// Topology has been changed
			auto oldInner = inner_;
			inner_ = newInner;
			Engine::OnNodeShift(*this, *oldInner, *newInner, turn);
			return ETickResult::invalidated;
		}

		TDomain::Log().template Append<NodeEvaluateBeginEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());
		TInner newValue = inner_->Value();
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

	virtual int DependencyCount() const override	{ return 2; }

private:
	SignalNodePtr<TDomain,TOuter>	outer_;
	SignalNodePtr<TDomain,TInner>	inner_;
};

// ---
}