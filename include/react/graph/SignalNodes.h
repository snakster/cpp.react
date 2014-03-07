#pragma once

#include <memory>
#include <functional>
#include <tuple>

#include "GraphBase.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace react {

////////////////////////////////////////////////////////////////////////////////////////
/// SignalNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename S
>
class SignalNode : public ReactiveNode<D,S,S>
{
public:
	typedef std::shared_ptr<SignalNode> NodePtrT;
	typedef std::weak_ptr<SignalNode>	NodeWeakPtrT;

	explicit SignalNode(bool registered) :
		ReactiveNode(true)
	{
		if (!registered)
			registerNode();
	}

	template <typename T>
	SignalNode(T&& value, bool registered) :
		ReactiveNode{ true },
		value_{ std::forward<T>(value) }
	{
		if (!registered)
			registerNode();
	}

	virtual const char* GetNodeType() const override { return "SignalNode"; }

	virtual ETickResult Tick(void* turnPtr) override
	{
		REACT_ASSERT(false, "Don't tick SignalNode\n");
		return ETickResult::none;
	}

	S Value() const
	{
		return value_;
	}

	S operator()(void) const
	{
		return Value();
	}

	const S& ValueRef() const
	{
		return value_;
	}

protected:
	S value_;
};

template <typename D, typename S>
using SignalNodePtr = typename SignalNode<D,S>::NodePtrT;

template <typename D, typename S>
using SignalNodeWeakPtr = typename SignalNode<D,S>::NodeWeakPtrT;

////////////////////////////////////////////////////////////////////////////////////////
/// VarNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename S
>
class VarNode : public SignalNode<D,S>
{
public:
	template <typename T>
	VarNode(T&& value, bool registered) :
		SignalNode<D,S>(std::forward<T>(value), true)
	{
		if (!registered)
			registerNode();
	}

	virtual const char* GetNodeType() const override { return "VarNode"; }

	virtual ETickResult Tick(void* turnPtr) override
	{
		if (! impl::Equals(value_, newValue_))
		{
			typedef typename D::Engine::TurnInterface TurnInterface;
			TurnInterface& turn = *static_cast<TurnInterface*>(turnPtr);

			value_ = newValue_;
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
	void AddInput(V&& newValue)
	{
		newValue_ = std::forward<V>(newValue);
	}

private:
	S		newValue_;
};

////////////////////////////////////////////////////////////////////////////////////////
/// ValNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename S
>
class ValNode : public SignalNode<D,S>
{
public:
	template <typename T>
	ValNode(T&& value, bool registered) :
		SignalNode<D,S>(std::forward<T>(value), true)
	{
		if (!registered)
			registerNode();
	}

	virtual const char* GetNodeType() const override { return "ValNode"; }

	virtual ETickResult Tick(void* turnPtr) override
	{
		REACT_ASSERT(false, "Don't tick the ValNode\n");
		return ETickResult::none;
	}
};

////////////////////////////////////////////////////////////////////////////////////////
/// FunctionNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename S,
	typename ... TArgs
>
class FunctionNode : public SignalNode<D,S>
{
public:
	FunctionNode(const SignalNodePtr<D,TArgs>& ... args, std::function<S(TArgs ...)> func, bool registered) :
		SignalNode<D, S>(true),
		deps_{ make_tuple(args ...) },
		func_{ func }
	{
		if (!registered)
			registerNode();

		value_ = func_(args->Value() ...);

		REACT_EXPAND_PACK(Engine::OnNodeAttach(*this, *args));
	}

	~FunctionNode()
	{
		apply
		(
			[this] (const SignalNodePtr<D,TArgs>& ... args)
			{
				REACT_EXPAND_PACK(Engine::OnNodeDetach(*this, *args));
			},
			deps_
		);
	}

	virtual const char* GetNodeType() const override { return "FunctionNode"; }

	virtual ETickResult Tick(void* turnPtr) override
	{
		typedef typename D::Engine::TurnInterface TurnInterface;
		TurnInterface& turn = *static_cast<TurnInterface*>(turnPtr);

		D::Log().template Append<NodeEvaluateBeginEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());
		S newValue = evaluate();
		D::Log().template Append<NodeEvaluateEndEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());

		if (! impl::Equals(value_, newValue))
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
	std::tuple<SignalNodePtr<D,TArgs> ...>	deps_;
	std::function<S(TArgs ...)>				func_;

	S evaluate() const
	{
		return apply(func_, apply(unpackValues, deps_));
	}

	static inline auto unpackValues(const SignalNodePtr<D,TArgs>& ... args) -> std::tuple<TArgs ...>
	{
		return std::make_tuple(args->ValueRef() ...);
	}
};

////////////////////////////////////////////////////////////////////////////////////////
/// FlattenNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename TOuter,
	typename TInner
>
class FlattenNode : public SignalNode<D,TInner>
{
public:
	FlattenNode(const SignalNodePtr<D,TOuter>& outer, const SignalNodePtr<D,TInner>& inner, bool registered) :
		SignalNode<D, TInner>(inner->Value(), true),
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
		typedef typename D::Engine::TurnInterface TurnInterface;
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

		D::Log().template Append<NodeEvaluateBeginEvent>(GetObjectId(*this), turn.Id(), std::this_thread::get_id().hash());
		TInner newValue = inner_->Value();
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

	virtual int DependencyCount() const override	{ return 2; }

private:
	SignalNodePtr<D,TOuter>	outer_;
	SignalNodePtr<D,TInner>	inner_;
};

// ---
}