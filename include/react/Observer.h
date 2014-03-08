#pragma once

#include "react/Defs.h"

#include <memory>
#include <unordered_map>

#include "react/graph/ObserverNodes.h"

////////////////////////////////////////////////////////////////////////////////////////
REACT_BEGIN

template <typename T>
class Reactive;

template <typename D, typename S>
class RSignal;

template <typename D, typename E>
class REvents;

enum class EventToken;

////////////////////////////////////////////////////////////////////////////////////////
/// RObserver
////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class RObserver
{
public:
	using SubjectT = REACT_IMPL::NodeBase<D>;
	using ObserverNodeT = REACT_IMPL::ObserverNode<D>;

	RObserver() :
		ptr_{ nullptr },
		subject_{ nullptr }
	{
	}

	RObserver(ObserverNodeT* ptr, const std::shared_ptr<SubjectT>& subject) :
		ptr_{ ptr },
		subject_{ subject }
	{
	}

	const ObserverNodeT* GetPtr() const
	{
		return ptr_;
	}

	bool Detach()
	{
		if (ptr_ == nullptr)
			return false;

		D::Observers().Unregister(ptr_);
		return true;
	}

private:
	// Ownership managed by registry
	ObserverNodeT*		ptr_;

	// While the observer handle exists, the subject is not destroyed
	std::shared_ptr<SubjectT>	subject_;
};

REACT_END

REACT_IMPL_BEGIN

////////////////////////////////////////////////////////////////////////////////////////
/// ObserverRegistry
////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class ObserverRegistry
{
public:
	typedef NodeBase<D>	SubjectT;

private:
	class Entry_
	{
	public:
		Entry_(std::unique_ptr<IObserverNode>&& obs, SubjectT* aSubject) :
			obs_{ std::move(obs) },
			Subject{ aSubject }
		{
		}

		Entry_(Entry_&& other) :
			obs_(std::move(other.obs_)),
			Subject(other.Subject)
		{
		}

		SubjectT*	Subject;

	private:
		// Manage lifetime
		std::unique_ptr<IObserverNode>	obs_;
	};

public:
	void Register(std::unique_ptr<IObserverNode>&& obs, SubjectT* subject)
	{
		auto* raw = obs.get();
		observerMap_.emplace(raw, Entry_(std::move(obs),subject));
	}

	void Unregister(IObserverNode* obs)
	{
		obs->Detach();
		observerMap_.erase(obs);
	}

	void UnregisterFrom(SubjectT* subject)
	{
		auto it = observerMap_.begin();
		while (it != observerMap_.end())
		{
			if (it->second.Subject == subject)
			{
				it->first->Detach();
				it = observerMap_.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

private:
	std::unordered_map<IObserverNode*,Entry_>	observerMap_;
};

REACT_IMPL_END

REACT_BEGIN

////////////////////////////////////////////////////////////////////////////////////////
/// Observe
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename F,
	typename TArg
>
inline auto Observe(const RSignal<D,TArg>& subject, F&& func)
	-> RObserver<D>
{
	std::unique_ptr< REACT_IMPL::ObserverNode<D>> pUnique(
		new  REACT_IMPL::SignalObserverNode<D,TArg>(
			subject.GetPtr(), std::forward<F>(func), false));

	auto* raw = pUnique.get();

	D::Observers().Register(std::move(pUnique), subject.GetPtr().get());

	return RObserver<D>(raw, subject.GetPtr());
}

template
<
	typename D,
	typename F,
	typename TArg,
	typename = std::enable_if<
		! std::is_same<TArg,EventToken>::value>::type
>
inline auto Observe(const REvents<D,TArg>& subject, F&& func)
	-> RObserver<D>
{
	std::unique_ptr< REACT_IMPL::ObserverNode<D>> pUnique(
		new  REACT_IMPL::EventObserverNode<D,TArg>(
			subject.GetPtr(), std::forward<F>(func), false));

	auto* raw = pUnique.get();

	D::Observers().Register(std::move(pUnique), subject.GetPtr().get());

	return RObserver<D>(raw, subject.GetPtr());
}

template
<
	typename D,
	typename F
>
inline auto Observe(const REvents<D,EventToken>& subject, F&& func)
	-> RObserver<D>
{
	std::unique_ptr< REACT_IMPL::ObserverNode<D>> pUnique(
		new  REACT_IMPL::EventObserverNode<D,EventToken>(
			subject.GetPtr(), [func] (EventToken _) { func(); }, false));

	auto* raw = pUnique.get();

	D::Observers().Register(std::move(pUnique), subject.GetPtr().get());

	return RObserver<D>(raw, subject.GetPtr());
}

////////////////////////////////////////////////////////////////////////////////////////
/// DetachAllObservers
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	template <typename Domain_, typename Val_> class TNode,
	typename TArg
>
inline void DetachAllObservers(const Reactive<TNode<D,TArg>>& subject)
{
	D::Observers().UnregisterFrom(subject.GetPtr().get());
}

////////////////////////////////////////////////////////////////////////////////////////
/// DetachThisObserver
////////////////////////////////////////////////////////////////////////////////////////
inline void DetachThisObserver()
{
	REACT_IMPL::current_observer_state_::shouldDetach = true;
}

REACT_END