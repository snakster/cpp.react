#pragma once

#include <atomic>
#include <vector>

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

class Node;

using std::atomic;
using std::vector;
using tbb::task_group;
using tbb::queuing_mutex;
using tbb::spin_mutex;

////////////////////////////////////////////////////////////////////////////////////////
/// Turn
////////////////////////////////////////////////////////////////////////////////////////
class Turn :
	public TurnBase,
	public ExclusiveTurnManager::ExclusiveTurn
{
public:
	using SourceIdSetT = SourceIdSet<ObjectId>;

	Turn(TurnIdT id, TurnFlagsT flags);

	void AddSourceId(ObjectId id);

	SourceIdSetT& Sources()		{ return sources_; }

private:
	SourceIdSetT	sources_;
};

////////////////////////////////////////////////////////////////////////////////////////
/// Node
////////////////////////////////////////////////////////////////////////////////////////
class Node : public IReactiveNode
{
public:
	typedef Turn::SourceIdSetT	SourceIdSetT;

	typedef queuing_mutex	NudgeMutexT;
	typedef spin_mutex		ShiftMutexT;

	Node();

	void AddSourceId(ObjectId id);

	void AttachSuccessor(Node& node);
	void DetachSuccessor(Node& node);

	void Destroy();

	void Pulse(Turn& turn, bool updated);
	void Shift(Node& oldParent, Node& newParent, Turn& turn);

	bool IsDependency(Turn& turn);
	bool CheckCurrentTurn(Turn& turn);

	void Nudge(Turn& turn, bool update, bool invalidate);

	void CheckForCycles(ObjectId startId) const;

private:
	enum
	{
		kFlag_Visited =		1 << 0,
		kFlag_Updated =		1 << 1,
		kFlag_Invaliated =	1 << 2
	};

	NodeVector<Node>	predecessors_;
	NodeVector<Node>	successors_;

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
	public IReactiveEngine<Node,Turn>,
	public ExclusiveTurnManager
{
public:
	void SourceSetEngine::OnNodeCreate(Node& node);

	void OnNodeAttach(Node& node, Node& parent);
	void OnNodeDetach(Node& node, Node& parent);

	void OnNodeDestroy(Node& node);

	void OnTurnAdmissionStart(Turn& turn);
	void OnTurnAdmissionEnd(Turn& turn);

	void OnTurnInputChange(Node& node, Turn& turn);
	void OnTurnPropagate(Turn& turn);

	void OnNodePulse(Node& node, Turn& turn);
	void OnNodeIdlePulse(Node& node, Turn& turn);
	void OnNodeShift(Node& node, Node& oldParent, Node& newParent, Turn& turn);

private:
	vector<Node*>	changedInputs_;
};

} // ~namespace react::source_set_impl

using source_set_impl::SourceSetEngine;

} // ~namespace react