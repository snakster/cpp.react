#pragma once

#include "react/Defs.h"

#include <functional>
#include <memory>

////////////////////////////////////////////////////////////////////////////////////////
REACT_BEGIN

////////////////////////////////////////////////////////////////////////////////////////
/// Reactive
////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
class Reactive
{
public:
	using Domain = typename T::Domain;

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

REACT_END

REACT_IMPL_BEGIN

template <typename L, typename R>
bool Equals(const L& lhs, const R& rhs)
{
	return lhs == rhs;
}

REACT_IMPL_END