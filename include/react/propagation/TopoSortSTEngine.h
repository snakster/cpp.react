#pragma once

#include "tbb/queuing_rw_mutex.h"

#include "EngineBase.h"
#include "react/ReactiveDomain.h"
#include "react/common/Containers.h"
#include "react/common/TopoQueue.h"
#include "react/common/Types.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace react {
namespace topo_sort_st_impl {

using tbb::queuing_rw_mutex;

////////////////////////////////////////////////////////////////////////////////////////
/// Node
////////////////////////////////////////////////////////////////////////////////////////
class Node : public IReactiveNode
{
public:
	Node();

	int		Level;
	int		NewLevel;
	bool	Queued;

	NodeVector<Node>	Successors;
};

////////////////////////////////////////////////////////////////////////////////////////
/// Turn
////////////////////////////////////////////////////////////////////////////////////////
class Turn : public TurnBase<Turn>
{
public:
	explicit Turn(TransactionData<Turn>& transactionData);
};

////////////////////////////////////////////////////////////////////////////////////////
/// TopoSortSTEngine
////////////////////////////////////////////////////////////////////////////////////////
class TopoSortSTEngine : public IReactiveEngine<Node,Turn>
{
public:
	typedef queuing_rw_mutex			GraphMutex;
	typedef TopoQueue<Node>	TopoQueue;

	TopoSortSTEngine();

	void OnNodeAttach(Node& node, Node& parent);
	void OnNodeDetach(Node& node, Node& parent);

	void OnTransactionCommit(TransactionData<Turn>& transaction);

	void OnNodePulse(Node& node, Turn& turn);
	void OnNodeShift(Node& node, Node& oldParent, Node& newParent, Turn& turn);

private:
	void processChildren(Node& node, Turn& turn);
	void invalidateSuccessors(Node& node);

	GraphMutex		graphMutex_;
	TopoQueue		scheduledNodes_;
};

} // ~namespace react::topo_sort_impl

using topo_sort_st_impl::TopoSortSTEngine;

} // ~namespace react
