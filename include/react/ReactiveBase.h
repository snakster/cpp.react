#pragma once

#include <functional>
#include <memory>

////////////////////////////////////////////////////////////////////////////////////////
namespace react {

////////////////////////////////////////////////////////////////////////////////////////
/// Reactive_
////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
class Reactive_
{
public:
	typedef typename T::Domain			Domain;

	Reactive_() :
		ptr_{ nullptr }
	{
	}

	explicit Reactive_(const std::shared_ptr<T>& ptr) :
		ptr_{ ptr }
	{
	}

	explicit Reactive_(std::shared_ptr<T>&& ptr) :
		ptr_{ std::move(ptr) }
	{
	}

	const std::shared_ptr<T>& GetPtr() const
	{
		return ptr_;
	}

	bool Equals(const Reactive_& other) const
	{
		return ptr_ == other.ptr_;
	}

protected:
	std::shared_ptr<T> ptr_;
};

// ---
}