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
struct Turn :
	public TurnBase<Turn>,
	public ExclusiveSequentialTransaction<Turn>
{
public:
	explicit Turn(TransactionData<Turn>& transactionData);
};

////////////////////////////////////////////////////////////////////////////////////////
/// Node
////////////////////////////////////////////////////////////////////////////////////////
class Node : public IReactiveNode
{
public:
	typedef spin_mutex	ShiftMutexT;

	Node();

	ShiftMutexT			ShiftMutex;
	NodeVector<Node>	Successors;

	atomic<int16_t>		Counter;
	atomic<bool>		ShouldUpdate;

	uint	LastTurnId;
};

////////////////////////////////////////////////////////////////////////////////////////
/// ELMEngine
////////////////////////////////////////////////////////////////////////////////////////
class ELMEngine :
	public IReactiveEngine<Node,Turn>,
	public ExclusiveSequentialTransactionEngine<Turn>
{
public:
	typedef Node::ShiftMutexT	NodeShiftMutexT;
	typedef set<Node*>			NodeSet;

	virtual void OnNodeCreate(NodeInterface& node);
	virtual void OnNodeDestroy(NodeInterface& node);

	virtual void OnNodeAttach(Node& node, Node& parent);
	virtual void OnNodeDetach(Node& node, Node& parent);

	virtual void OnTransactionCommit(TransactionData<Turn>& transaction);

	virtual void OnInputNodeAdmission(Node& node, Turn& turn);

	virtual void OnNodePulse(Node& node, Turn& turn);
	virtual void OnNodeIdlePulse(Node& node, Turn& turn);

	virtual void OnNodeShift(Node& node, Node& oldParent, Node& newParent, Turn& turn);

private:
	void processChild(Node& node, bool update, Turn& turn);
	void nudgeChildren(Node& parent, bool update, Turn& turn);

	task_group	tasks_;

	NodeSet		inputNodes_;
	NodeSet		unchangedInputNodes_;
};

} // ~namespace react::elm_impl

using elm_impl::ELMEngine;

} // ~namespace react