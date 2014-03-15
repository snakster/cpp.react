
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstdio>
#include <vector>

#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"

#include "Types.h"

/*********************************/ REACT_IMPL_BEGIN /*********************************/

////////////////////////////////////////////////////////////////////////////////////////
/// GraphData
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TNode,
	typename TChunk,
	int INIT_NODE_COUNT
>
class GraphData
{
public:
	enum
	{
		kNodesPerChunk = sizeof(TChunk) * 8,
		kOffsetMask = kNodesPerChunk - 1
	};

	static_assert(kNodesPerChunk != 0, "kNodesPerChunk is 0");
	static_assert((INIT_NODE_COUNT % kNodesPerChunk) == 0, "INIT_NODE_COUNT must be multiple of kNodesPerChunk");

private:
	class Chunk
	{
	public:
		Chunk() : data_(0) {}

		inline bool IsSet(uint offset) const	{ return (data_ >> offset)  & 1; }
		inline void Set(uint offset)			{ data_ |= (((TChunk)1) << offset); }
		inline void Clear(uint offset)			{ data_ &= ~(((TChunk)1) << offset); }

		inline void Reset()						{ data_ = 0; }

		Chunk& operator=(const TChunk data)		{ data_ = data; return *this; }
		operator TChunk() const					{ return data_; }

		void Dump() const
		{
			TChunk t = data_;
			for (int i=0; i<sizeof(TChunk) * 8; i++)
			{
				printf("%u", (t & 0x1));
				t >>= 1;
			}
			printf(" ");
		}

	private:
		TChunk	data_;
	};

public:

	enum class EEntryType
	{
		Free = 0,
		Node,
		Buffer
	};

	class Entry
	{
	public:
		Entry() :
			Node(nullptr),
			type_(EEntryType::Free)
		{
		}

		TNode*		Node;

	private:
		EEntryType	type_;

		friend class GraphData;
	};

	class Iterator
	{
	public:
		Iterator(const Chunk* curChunk) :
			curIndex_(0),
			curOffset_(0),
			curChunk_(curChunk)
		{
		}

		Iterator(const Chunk* curChunk, uint index, uint offset) :
			curIndex_(index),
			curOffset_(offset),
			curChunk_(curChunk)
		{
		}

		inline bool operator<(const Iterator& other) const
		{
			return	(curChunk_ < other.curChunk_) ||
					(curChunk_ == other.curChunk_ && curOffset_ < other.curOffset_);
		}

		inline Iterator& operator+=(const Iterator& other)
		{
			curChunk_ += other.curChunk_;
			curOffset_+= other.curOffset_;
			curIndex_ += other.curIndex_;

			if (curOffset_ >= kNodesPerChunk)
			{
				curOffset_ -= kNodesPerChunk;
				++curChunk_;
			}

			return *this;
		}

		inline Iterator operator+(const Iterator& other)
		{
			Iterator result;
			result += other;
			return result;
		}

		inline Iterator& operator++()
		{
			curIndex_++;

			if (curOffset_ < kNodesPerChunk - 1)
			{
				curOffset_++;
			}
			else
			{
				curOffset_ = 0;
				++curChunk_;
			}
			return *this;
		}

		inline Iterator operator++(int)
		{
			Iterator tmp(*this);
			operator++();
			return tmp;
		}

		inline bool IsReachable() const
		{
			return curChunk_->IsSet(curOffset_);
		}

		inline uint Index() const
		{
			return curIndex_;
		}

	private:
		uint		curIndex_;
		uint		curOffset_;

		const Chunk*	curChunk_;
	};

	GraphData() :
		entryCapacity_(INIT_NODE_COUNT),
		chunksPerEntry_(entryCapacity_ / kNodesPerChunk),
		chunks_(new Chunk[entryCapacity_ * chunksPerEntry_]),
		nextIndex_(0),
		entries_(entryCapacity_)
	{
	}

	~GraphData()
	{
		delete[] chunks_;
	}

	uint EntryCapacity() const
	{
		return entryCapacity_;
	}

	bool IsNode(uint index) const
	{
		return entries_[index].type_ == EEntryType::Node;
	}

	bool IsBuffer(uint index) const
	{
		return entries_[index].type_ == EEntryType::Buffer;
	}

	inline bool IsReachable(uint from, uint to) const
	{
		uint vIndex = (from * entryCapacity_ + to);
		return chunks_[vIndex / kNodesPerChunk].IsSet(vIndex & kOffsetMask);
	}

	inline void SetReachable(uint from, uint to)
	{
		uint vIndex = (from * entryCapacity_ + to);
		chunks_[vIndex / kNodesPerChunk].Set(vIndex & kOffsetMask);
	}

	inline void ClearReachable(uint from, uint to)
	{
		uint vIndex = (from * entryCapacity_ + to);
		chunks_[vIndex / kNodesPerChunk].Clear(vIndex & kOffsetMask);
	}	

	inline Entry& GetEntry(uint index)
	{
		return entries_[index];
	}

	uint InitNode(TNode& node)
	{
		// New index
		uint index = requestIndex();

		entries_[index].type_ = EEntryType::Node;
		entries_[index].Node = &node;

		SetReachable(index,index);

		return index;
	}

	uint InitBuffer()
	{
		int index = requestIndex();

		entries_[index].type_ = EEntryType::Buffer;

		return index;
	}

	void ReleaseNode(uint index)
	{
		clearIndex(index);

		entries_[index].Node = nullptr;
	}

	void ReleaseBuffer(uint index)
	{
		clearIndex(index);
	}

	// Single-threaded
	template <typename F>
	void BufferOp(uint dstIndex, uint srcIndex, const F& op)
	{
		auto* srcStart = &chunks_[srcIndex * chunksPerEntry_];
		auto* srcEnd = srcStart + chunksPerEntry_;

		auto* dst = &chunks_[dstIndex * chunksPerEntry_];

		while (srcStart != srcEnd)
		{
			*dst = op(*dst, *srcStart);
			srcStart++; dst++;
		}
	}

	// Parallel
	template <typename F>
	void ParallelBufferOp(uint dstIndex, uint srcIndex, uint grainSize, const F& op)
	{
		Chunk* srcStart = &chunks_[srcIndex * chunksPerEntry_];
		Chunk* srcEnd = srcStart + chunksPerEntry_;

		const int offset = chunksPerEntry_ * (dstIndex - srcIndex);

		tbb::parallel_for(tbb::blocked_range<Chunk*>(srcStart, srcEnd, grainSize),
			[&] (const tbb::blocked_range<Chunk*>& range)
			{
				const Chunk* src = range.begin();
				Chunk* dst = range.begin() + offset;

				while (src != range.end())
				{
					*dst = op(*dst, *src);
					src++; dst++;
				}
			}
		);
	}

	typedef std::pair<Iterator,Iterator>	Range;

	Range ReachableRange(uint nodeIndex) const
	{
		return std::make_pair(
			Iterator(&chunks_[chunksPerEntry_ * nodeIndex]),
			Iterator(&chunks_[chunksPerEntry_ * (nodeIndex+1)]));
	}

	Range ReachableRange(uint nodeIndex, uint startIndex, uint endIndex) const
	{
		Chunk*	startChunk = &chunks_[chunksPerEntry_ * nodeIndex + (startIndex / kNodesPerChunk)];
		uint	startOffset = startIndex & kOffsetMask;

		Chunk*	endChunk = &chunks_[chunksPerEntry_ * nodeIndex + (endIndex / kNodesPerChunk)];
		uint	endOffset = endIndex & kOffsetMask;

		return std::make_pair(
			Iterator(startChunk, startIndex, startOffset),
			Iterator(endChunk, endIndex, endOffset));
	}

	void Dump()
	{
		printf("Dump graph:\n");
		for (uint i=0; i<entryCapacity_; i++)
		{
			for (uint j=0; j<chunksPerEntry_; j++)
			{
				chunks_[i * chunksPerEntry_ + j].Dump();
			}
			printf("\n");
		}
		printf("\n");
	}

private:
	typedef std::vector<Entry>	EntryVector;

	uint		entryCapacity_;
	uint		chunksPerEntry_;

	uint		nextIndex_;

	EntryVector		entries_;
	Chunk*			chunks_;

	uint requestIndex()
	{
		int cycleMarker = (nextIndex_ - 1) % entryCapacity_;
		if (cycleMarker < 0)
			cycleMarker += entryCapacity_;

		auto it = entries_.begin() + nextIndex_;

		while (cycleMarker != nextIndex_)
		{
			if (it->type_ == EEntryType::Free)
				return nextIndex_;
			
			if (it != entries_.end() - 1)
			{
				nextIndex_++;
				it++;
			}
			else
			{
				nextIndex_ = 0;
				it = entries_.begin();
			}
		}

		// All in use => grow
		nextIndex_ = entryCapacity_;
		grow();

		entries_.resize(entryCapacity_);

		return nextIndex_;
	}

	void clearIndex(uint index)
	{
		// Entry
		entries_[index].type_ = EEntryType::Free;

		// Matrix row
		{
			Chunk* start = &chunks_[index * chunksPerEntry_];
			Chunk* end = start + chunksPerEntry_;
			while (start != end)
			{
				start->Reset();
				start++;
			}
		}

		// Matrix col
		{
			const uint offset = index & kOffsetMask;

			Chunk* p = &chunks_[index / kNodesPerChunk];
			for (uint i=0; i<entryCapacity_; i++)
			{
				p->Clear(offset);
				p += chunksPerEntry_;
			}
		}
	}

	void grow()
	{
		const uint	newEntryCount = entryCapacity_ << 1;
		const uint	newChunksPerEntry = newEntryCount / kNodesPerChunk;
		const uint	dataCountDelta = newChunksPerEntry - chunksPerEntry_;

		Chunk*	newChunks =	new Chunk[newEntryCount * newChunksPerEntry];

		uint insertIndex = 0;

		for (uint i=0; i<entryCapacity_; i++)
		{
			for (uint j=0; j<chunksPerEntry_; j++, insertIndex++)
				newChunks[insertIndex] = chunks_[i * chunksPerEntry_ + j];
			insertIndex += dataCountDelta;
		}

		delete[] chunks_;

		chunks_ = newChunks;
		entryCapacity_ = newEntryCount;
		chunksPerEntry_ = newChunksPerEntry;
	}

	void shrink()
	{
		// TODO
	}


};

/**********************************/ REACT_IMPL_END /**********************************/