#pragma once

#include <functional>
#include <memory>

////////////////////////////////////////////////////////////////////////////////////////
namespace react {

////////////////////////////////////////////////////////////////////////////////////////
/// Reactive
////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
class Reactive
{
public:
	typedef typename T::Domain			Domain;

	Reactive() :
		ptr_{ nullptr }
	{
	}

	explicit Reactive(const std::shared_ptr<T>& ptr) :
		ptr_{ ptr }
	{
	}

	explicit Reactive(std::shared_ptr<T>&& ptr) :
		ptr_{ std::move(ptr) }
	{
	}

	const std::shared_ptr<T>& GetPtr() const
	{
		return ptr_;
	}

	bool Equals(const Reactive& other) const
	{
		return ptr_ == other.ptr_;
	}

protected:
	std::shared_ptr<T> ptr_;
};

////////////////////////////////////////////////////////////////////////////////////////
namespace impl
{

template <typename L, typename R>
bool Equals(L lhs, R rhs)
{
	return lhs == rhs;
}

} // ~namespace react::impl

} // ~namespace react