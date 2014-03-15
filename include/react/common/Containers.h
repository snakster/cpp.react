
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/Defs.h"

#include <algorithm>
#include <array>
#include <vector>

/*********************************/ REACT_IMPL_BEGIN /*********************************/

////////////////////////////////////////////////////////////////////////////////////////
/// NodeVector
////////////////////////////////////////////////////////////////////////////////////////
template <typename TNode>
class NodeVector
{
private:
	typedef std::vector<TNode*>	DataT;

public:
	void Add(TNode& node)
	{
		data_.push_back(&node);
	}

	void Remove(const TNode& node)
	{
		data_.erase(std::find(data_.begin(), data_.end(), &node));
	}

    typedef typename DataT::iterator		iterator;
    typedef typename DataT::const_iterator	const_iterator;

    iterator	begin()	{ return data_.begin(); }
	iterator	end()	{ return data_.end(); }

    const_iterator begin() const	{ return data_.begin(); }
    const_iterator end() const		{ return data_.end(); }

private:
	DataT	data_;
};

////////////////////////////////////////////////////////////////////////////////////////
/// NodePtrBuffer
////////////////////////////////////////////////////////////////////////////////////////
template <int N, typename T>
class NodePtrBuffer
{
	typedef std::array<T*,N>	DataT;

public:
	bool Push(T* e)
	{
		ASSERT_(size_ < N);
		nodes_[size_++] = e;
		return size_ < N;
	}

	void Clear()
	{
		size_ = 0;
	}

	int Size()
	{
		return size_;
	}

    typedef typename DataT::iterator		iterator;
    typedef typename DataT::const_iterator	const_iterator;

    iterator	begin()	{ return nodes_.begin(); }
	iterator	end()	{ return nodes_.begin() + size_; }

    const_iterator begin() const	{ return nodes_.begin(); }
    const_iterator end() const		{ return nodes_.begin() + size_; }

private:
	int		size_ = 0;	
	DataT	nodes_;
};

/**********************************/ REACT_IMPL_END /**********************************/