#pragma once

#include <mutex>
#include <condition_variable>

namespace react {

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
	inline void Run(F func)
	{// mutex_
		std::lock_guard<std::mutex> scopedLock(mutex_);
		func(blocked_);
	}// ~mutex_

	template <typename F>
	inline bool RunIfBlocked(F func)
	{// mutex_
		std::lock_guard<std::mutex> scopedLock(mutex_);

		if (!blocked_)
			return false;

		func();
		return true;			
	}// ~mutex_

	template <typename F>
	inline bool RunIfUnblocked(F func)
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

} // ~namespace react