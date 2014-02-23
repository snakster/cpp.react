#pragma once

#include <memory>
#include <unordered_map>

#include "react/graph/ObserverNodes.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace react {

template <typename T>
class Reactive_;

template <typename TDomain, typename TValue>
class Signal_;

template <typename TDomain, typename TValue>
class Events_;

////////////////////////////////////////////////////////////////////////////////////////
/// Observer_
////////////////////////////////////////////////////////////////////////////////////////
template <typename TDomain>
class Observer_
{
public:
	typedef NodeBase<TDomain>	SubjectT;

	Observer_() :
		ptr_{ nullptr },
		subject_{ nullptr }
	{
	}

	Observer_(ObserverNode<TDomain>* ptr, const std::shared_ptr<SubjectT>& subject) :
		ptr_{ ptr },
		subject_{ subject }
	{
	}

	const ObserverNode<TDomain>* GetPtr() const
	{
		return ptr_;
	}

	bool Detach()
	{
		if (ptr_ == nullptr)
			return false;

		TDomain::Observers().Unregister(ptr_);
		return true;
	}

private:
	// Ownership managed by registry
	ObserverNode<TDomain>*		ptr_;

	// While the observer handle exists, the subject is not destroyed
	std::shared_ptr<SubjectT>	subject_;
};

////////////////////////////////////////////////////////////////////////////////////////
/// ObserverRegistry
////////////////////////////////////////////////////////////////////////////////////////
template <typename TDomain>
class ObserverRegistry
{
public:
	typedef NodeBase<TDomain>	SubjectT;

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

////////////////////////////////////////////////////////////////////////////////////////
/// Observe
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename TFunc,
	typename TArg
>
inline auto Observe(const Signal_<TDomain,TArg>& subject, const TFunc& func)
	-> Observer_<TDomain>
{
	std::unique_ptr<ObserverNode<TDomain>> pUnique(
		new SignalObserverNode<TDomain,TArg>(subject.GetPtr(), func, false));

	auto* raw = pUnique.get();

	TDomain::Observers().Register(std::move(pUnique), subject.GetPtr().get());

	return Observer_<TDomain>(raw, subject.GetPtr());
}

template
<
	typename TDomain,
	typename TFunc,
	typename TArg
>
inline auto Observe(const Events_<TDomain,TArg>& subject, const TFunc& func)
	-> Observer_<TDomain>
{
	std::unique_ptr<ObserverNode<TDomain>> pUnique(
		new EventObserverNode<TDomain,TArg>(subject.GetPtr(), func, false));

	auto* raw = pUnique.get();

	TDomain::Observers().Register(std::move(pUnique), subject.GetPtr().get());

	return Observer_<TDomain>(raw, subject.GetPtr());
}

////////////////////////////////////////////////////////////////////////////////////////
/// DetachAllObservers
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	template <typename Domain_, typename Val_> class TNode,
	typename TArg
>
inline void DetachAllObservers(const Reactive_<TNode<TDomain,TArg>>& subject)
{
	TDomain::Observers().UnregisterFrom(subject.GetPtr().get());
}

////////////////////////////////////////////////////////////////////////////////////////
/// DetachThisObserver
////////////////////////////////////////////////////////////////////////////////////////
inline void DetachThisObserver()
{
	current_observer_state_::shouldDetach = true;
}

// ---
}