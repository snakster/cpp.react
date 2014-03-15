
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/Defs.h"

#include <mutex>
#include <condition_variable>

/*********************************/ REACT_IMPL_BEGIN /*********************************/

class BlockingCondition
{
public:
	inline void Block()
	{// mutex_
		std::lock_guard<std::mutex> scopedLock(mutex_);
		blocked_ = true;
	}// ~mutex_

	inline void Unblock()
	{// mutex_
		std::lock_guard<std::mutex> scopedLock(mutex_);
		blocked_ = false;
		condition_.notify_all();
	}// ~mutex_

	inline void WaitForUnblock()
	{
		std::unique_lock<std::mutex> lock(mutex_);
		condition_.wait(lock, [this] { return !blocked_; });
	}

	inline bool IsBlocked()
	{// mutex_
		std::lock_guard<std::mutex> scopedLock(mutex_);
		return blocked_;
	}// ~mutex_

	template <typename F>
	auto Run(F&& func) -> decltype(func(false))
	{// mutex_
		std::lock_guard<std::mutex> scopedLock(mutex_);
		return func(blocked_);
	}// ~mutex_

	template <typename F>
	inline bool RunIfBlocked(F&& func)
	{// mutex_
		std::lock_guard<std::mutex> scopedLock(mutex_);

		if (!blocked_)
			return false;

		func();
		return true;			
	}// ~mutex_

	template <typename F>
	bool RunIfUnblocked(F&& func)
	{// mutex_
		std::lock_guard<std::mutex> scopedLock(mutex_);

		if (blocked_)
			return false;

		func();
		return true;			
	}// ~mutex_

private:
	std::mutex				mutex_;
	std::condition_variable	condition_;
	bool					blocked_ = false;
};

////////////////////////////////////////////////////////////////////////////////////////
/// ThreadLocalPtr
////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
class ThreadLocalStaticPtr
{
public:
	static T*	Get()			{ return ptr_; }
	static void	Set(T* ptr)		{ ptr_ = ptr; }
	static void	Reset()			{ ptr_ = nullptr; }
	static bool	IsNull()		{ return ptr_ == nullptr; }

private:
	ThreadLocalStaticPtr() {}

	static __declspec(thread) T*	ptr_;
};

template <typename T>
T*	ThreadLocalStaticPtr<T>::ptr_(nullptr);

/**********************************/ REACT_IMPL_END /**********************************/