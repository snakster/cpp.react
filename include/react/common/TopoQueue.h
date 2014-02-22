#pragma once

#include <algorithm>
#include <array>
#include <vector>

////////////////////////////////////////////////////////////////////////////////////////
namespace react {

////////////////////////////////////////////////////////////////////////////////////////
/// TopoQueue
////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
class TopoQueue
{
public:
	static bool LevelOrderOp(const T* lhs, const T* rhs)
	{
		return lhs->Level > rhs->Level;
	}

	void Push(T* node)
	{
		data_.push_back(node);
		std::push_heap(data_.begin(), data_.end(), LevelOrderOp);
	}

	void Pop()
	{
		std::pop_heap(data_.begin(), data_.end(), LevelOrderOp);
		data_.pop_back();
	}

	T* TopoQueue::Top() const
	{
		return data_.front();
	}

	bool TopoQueue::Empty() const
	{
		return data_.empty();
	}

	void Invalidate()
	{
		std::make_heap(data_.begin(), data_.end(), LevelOrderOp);
	}

private:
	std::vector<T*>	data_;
};

} // ~namespace react