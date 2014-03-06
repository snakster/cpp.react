#pragma once

#include <atomic>
#include <set>

#include "tbb/task_group.h"
#include "tbb/spin_mutex.h"

#include "react/ReactiveDomain.h"
#include "react/common/Containers.h"
#include "react/common/Types.h"
#include "react/propagation/EngineBase.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace react {
namespace elm_impl {

using std::atomic;
using std::set;
using tbb::task_group;
using tbb::spin_mutex;

////////////////////////////////////////////////////////////////////////////////////////
/// Turn
////////////////////////////////////////////////////////////////////////////////////////
class Turn :
	public TurnBase,
	public ExclusiveTurnManager::ExclusiveTurn
{
public:
	Turn(TurnIdT id, TurnFlagsT flags);
};

////////////////////////////////////////////////////////////////////////////////////////
/// Node
////////////////////////////////////////////////////////////////////////////////////////
class Node : public IReactiveNode
{
public:
	using ShiftMutexT = spin_mutex;

	Node();

	ShiftMutexT			ShiftMutex;
	NodeVector<Node>	Successors;

	atomic<int16_t>		Counter;
	atomic<bool>		ShouldUpdate;

	TurnIdT				LastTurnId;
};

////////////////////////////////////////////////////////////////////////////////////////
/// ELMEngine
////////////////////////////////////////////////////////////////////////////////////////
class ELMEngine :
	public IReactiveEngine<Node,Turn>,
	public ExclusiveTurnManager
{
public:
	using NodeShiftMutexT = Node::ShiftMutexT;
	using NodeSetT = set<Node*>;

	void OnNodeCreate(NodeInterface& node);
	void OnNodeDestroy(NodeInterface& node);

	void OnNodeAttach(Node& node, Node& parent);
	void OnNodeDetach(Node& node, Node& parent);

	void OnTurnAdmissionStart(Turn& turn);
	void OnTurnAdmissionEnd(Turn& turn);

	void OnTurnInputChange(Node& node, Turn& turn);
	void OnTurnPropagate(Turn& turn);

	void OnNodePulse(Node& node, Turn& turn);
	void OnNodeIdlePulse(Node& node, Turn& turn);

	void OnNodeShift(Node& node, Node& oldParent, Node& newParent, Turn& turn);

private:
	void processChild(Node& node, bool update, Turn& turn);
	void nudgeChildren(Node& parent, bool update, Turn& turn);

	task_group	tasks_;
	NodeSetT	inputNodes_;
};

} // ~namespace react::elm_impl

using elm_impl::ELMEngine;

} // ~namespace react