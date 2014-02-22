#pragma once

#include "tbb/queuing_mutex.h"
#include "tbb/spin_mutex.h"
#include "tbb/task_group.h"

#include "EngineBase.h"
#include "react/ReactiveDomain.h"
#include "react/common/Containers.h"
#include "react/common/SourceIdSet.h"
#include "react/common/Types.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace react {
namespace source_set_impl {

class SourceSetNode;

using std::atomic;
using tbb::task_group;
using tbb::queuing_mutex;
using tbb::spin_mutex;

////////////////////////////////////////////////////////////////////////////////////////
/// SourceSetTurn
////////////////////////////////////////////////////////////////////////////////////////
struct SourceSetTurn :
	public TurnBase<SourceSetTurn>,
	public ExclusiveSequentialTransaction<SourceSetTurn>
{
public:
	typedef SourceIdSet<ObjectId>	SourceIdSetT;

	explicit SourceSetTurn(TransactionData<SourceSetTurn>& transactionData);

	void AddSourceId(ObjectId id);

	SourceIdSetT& Sources()		{ return sources_; }

private:
	SourceIdSetT	sources_;
};

////////////////////////////////////////////////////////////////////////////////////////
/// SourceSetNode
////////////////////////////////////////////////////////////////////////////////////////
class SourceSetNode : public IReactiveNode
{
public:
	typedef SourceSetTurn::SourceIdSetT	SourceIdSetT;

	typedef queuing_mutex	NudgeMutexT;
	typedef spin_mutex		ShiftMutexT;

	SourceSetNode();

	void AddSourceId(ObjectId id);

	void AttachSuccessor(SourceSetNode& node);
	void DetachSuccessor(SourceSetNode& node);

	void Destroy();

	void Pulse(SourceSetTurn& turn, bool updated);
	void Shift(SourceSetNode& oldParent, SourceSetNode& newParent, SourceSetTurn& turn);

	bool IsDependency(SourceSetTurn& turn);
	bool CheckCurrentTurn(SourceSetTurn& turn);

	void Nudge(SourceSetTurn& turn, bool update, bool invalidate);

	void CheckForCycles(ObjectId startId) const;

private:
	enum
	{
		kFlag_Visited =		1 << 0,
		kFlag_Updated =		1 << 1,
		kFlag_Invaliated =	1 << 2
	};

	NodeVector<SourceSetNode>	predecessors_;
	NodeVector<SourceSetNode>	successors_;

	SourceIdSetT	sources_;

	uint			curTurnId_;
	
	short			tickThreshold_;
	uchar			flags_;

	NudgeMutexT		nudgeMutex_;
	ShiftMutexT		shiftMutex_;

	void invalidateSources();
};

////////////////////////////////////////////////////////////////////////////////////////
/// SourceSetEngine
////////////////////////////////////////////////////////////////////////////////////////
class SourceSetEngine :
	public IReactiveEngine<SourceSetNode,SourceSetTurn>,
	public ExclusiveSequentialTransactionEngine<SourceSetTurn>
{
public:
	void SourceSetEngine::OnNodeCreate(SourceSetNode& node);

	void OnNodeAttach(SourceSetNode& node, SourceSetNode& parent);
	void OnNodeDetach(SourceSetNode& node, SourceSetNode& parent);

	void OnNodeDestroy(SourceSetNode& node);

	void OnTransactionCommit(TransactionData<SourceSetTurn>& transaction);
	void OnInputNodeAdmission(SourceSetNode& node, SourceSetTurn& turn);

	void OnNodePulse(SourceSetNode& node, SourceSetTurn& turn);
	void OnNodeIdlePulse(SourceSetNode& node, SourceSetTurn& turn);
	void OnNodeShift(SourceSetNode& node, SourceSetNode& oldParent, SourceSetNode& newParent, SourceSetTurn& turn);
};

} // ~namespace react::source_set_impl

using source_set_impl::SourceSetEngine;

} // ~namespace react