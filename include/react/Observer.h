#pragma once

#include <memory>
#include <unordered_map>

#include "react/graph/ObserverNodes.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace react {

template <typename T>
class Reactive;

template <typename D, typename TValue>
class RSignal;

template <typename D, typename TValue>
class REvents;

////////////////////////////////////////////////////////////////////////////////////////
/// Observer_
////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class Observer_
{
public:
	typedef NodeBase<D>	SubjectT;

	Observer_() :
		ptr_{ nullptr },
		subject_{ nullptr }
	{
	}

	Observer_(ObserverNode<D>* ptr, const std::shared_ptr<SubjectT>& subject) :
		ptr_{ ptr },
		subject_{ subject }
	{
	}

	const ObserverNode<D>* GetPtr() const
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
	ObserverNode<D>*		ptr_;

	// While the observer handle exists, the subject is not destroyed
	std::shared_ptr<SubjectT>	subject_;
};

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

////////////////////////////////////////////////////////////////////////////////////////
/// Observe
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename TFunc,
	typename TArg
>
inline auto Observe(const RSignal<D,TArg>& subject, const TFunc& func)
	-> Observer_<D>
{
	std::unique_ptr<ObserverNode<D>> pUnique(
		new SignalObserverNode<D,TArg>(subject.GetPtr(), func, false));

	auto* raw = pUnique.get();

	D::Observers().Register(std::move(pUnique), subject.GetPtr().get());

	return Observer_<D>(raw, subject.GetPtr());
}

template
<
	typename D,
	typename TFunc,
	typename TArg
>
inline auto Observe(const REvents<D,TArg>& subject, const TFunc& func)
	-> Observer_<D>
{
	std::unique_ptr<ObserverNode<D>> pUnique(
		new EventObserverNode<D,TArg>(subject.GetPtr(), func, false));

	auto* raw = pUnique.get();

	D::Observers().Register(std::move(pUnique), subject.GetPtr().get());

	return Observer_<D>(raw, subject.GetPtr());
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
	current_observer_state_::shouldDetach = true;
}

} // ~namespace react