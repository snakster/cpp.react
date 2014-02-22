#pragma once

#include <set>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include "tbb/queuing_mutex.h"

#include "react/ReactiveDomain.h"

namespace react {

////////////////////////////////////////////////////////////////////////////////////////
/// IReactiveEngine
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TNodeInterface,
	typename TTurnInterface
>
struct IReactiveEngine
{
	typedef TNodeInterface	NodeInterface;
	typedef TTurnInterface	TurnInterface;

	void OnNodeCreate(NodeInterface& node)	{}
	void OnNodeDestroy(NodeInterface& node)	{}

	void OnNodeAttach(NodeInterface& node, NodeInterface& parent)		{}
	void OnNodeDetach(NodeInterface& node, NodeInterface& parent)		{}

	void OnTransactionCommit(const TransactionData<TurnInterface>& transaction)	{}

	void OnInputNodeAdmission(NodeInterface& node, TurnInterface& turn)	{}

	void OnNodePulse(NodeInterface& node, TurnInterface& turn)		{}
	void OnNodeIdlePulse(NodeInterface& node, TurnInterface& turn)	{}

	void OnNodeShift(NodeInterface& node, NodeInterface& oldParent, NodeInterface& newParent, TurnInterface& turn)	{}
};

////////////////////////////////////////////////////////////////////////////////////////
/// TurnBase
////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurnInterface>
class TurnBase
{
public:
	TurnBase(TransactionData<TTurnInterface>& transactionData) :
		transactionData_(transactionData)
	{
	}

	int Id() const		{ return transactionData_.Id(); }

	TransactionInput<TTurnInterface>& InputContinuation()
	{
		return transactionData_.NextInput();
	}

	void QueueForDetach(IObserverNode& obs)
	{
		transactionData_.DetachedObservers.push_back(&obs);
	}

private:
	TransactionData<TTurnInterface>&	transactionData_;
};

////////////////////////////////////////////////////////////////////////////////////////
/// ExclusiveSequentialTransaction
////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurnInterface>
class ExclusiveSequentialTransaction
{
public:
	typedef std::vector<ExclusiveSequentialTransaction*>	TransactionVector;

	explicit ExclusiveSequentialTransaction(TransactionInput<TTurnInterface>& input) :
		blocked_(false),
		successor_(nullptr),
		input_(input)
	{
	}

	void Append(ExclusiveSequentialTransaction& tr)
	{
		successor_ = &tr;
		tr.Block();
	}

	void Block()
	{
		// blockMutex_
		{
			std::lock_guard<std::mutex> scopedLock(blockMutex_);

			blocked_ = true;
		}
		// ~blockMutex_
	}

	void Unblock()
	{
		// blockMutex_
		{
			std::lock_guard<std::mutex> scopedLock(blockMutex_);

			blocked_ = false;
			blockCondition_.notify_all();
		}
		// ~blockMutex_
	}

	void WaitForUnblock()
	{
		std::unique_lock<std::mutex> lock(blockMutex_);

		while (blocked_)
			blockCondition_.wait(lock);
	}

	void Finish()
	{
		for (auto p : mergedTransactions_)
			p->Unblock();

		if (successor_)
			successor_->Unblock();
	}

	bool TryMerge(ExclusiveSequentialTransaction& other)
	{
		if ((input_.Flags() & allow_transaction_merging) && (other.input_.Flags() & allow_transaction_merging))
		{
			// blockMutex_
			{
				std::lock_guard<std::mutex> scopedLock(blockMutex_);

				// Already started?
				if (!blocked_)
					return false;

				other.Block();

				input_.Merge(other.input_);

				mergedTransactions_.push_back(&other);

				return true;
			}
			// ~blockMutex_
		}
		else
		{
			return false;
		}
	}

private:
	TransactionInput<TTurnInterface>&	input_;
	ExclusiveSequentialTransaction*		successor_;
	TransactionVector					mergedTransactions_;

	std::mutex					blockMutex_;
	std::condition_variable		blockCondition_;

	
	bool			blocked_;
};

////////////////////////////////////////////////////////////////////////////////////////
/// BlockingSeqEngine
////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurnInterface>
class ExclusiveSequentialTransactionEngine
{
public:
	bool BeginTransaction(ExclusiveSequentialTransaction<TTurnInterface>& tr)
	{
		bool merged = false;

		// sequenceMutex_
		{
			SeqMutex::scoped_lock	lock(sequenceMutex_);

			if (tail_)
			{
				merged = tail_->TryMerge(tr);

				if (!merged)
					tail_->Append(tr);
			}
		
			if (!merged)
				tail_ = &tr;
		}
		// ~sequenceMutex_

		tr.WaitForUnblock();

		return !merged;
	}

	void EndTransaction(ExclusiveSequentialTransaction<TTurnInterface>& tr)
	{
		// sequenceMutex_
		{
			SeqMutex::scoped_lock	lock(sequenceMutex_);

			tr.Finish();

			if (tail_ == &tr)
				tail_ = nullptr;
		}
		// ~sequenceMutex_

	}

private:
	typedef tbb::queuing_mutex		SeqMutex;

	SeqMutex	sequenceMutex_;

	ExclusiveSequentialTransaction<TTurnInterface>*	tail_;
};

// ---
}