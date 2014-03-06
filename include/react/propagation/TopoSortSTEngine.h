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
class Turn :
	public TurnBase
{
public:
	Turn(TurnIdT id, TurnFlagsT flags);
};

////////////////////////////////////////////////////////////////////////////////////////
/// TopoSortSTEngine
////////////////////////////////////////////////////////////////////////////////////////
class TopoSortSTEngine : public IReactiveEngine<Node,Turn>
{
public:
	typedef TopoQueue<Node>	TopoQueue;

	TopoSortSTEngine();

	void OnNodeAttach(Node& node, Node& parent);
	void OnNodeDetach(Node& node, Node& parent);

	void OnTurnAdmissionStart(Turn& turn);
	void OnTurnAdmissionEnd(Turn& turn);

	void OnTurnInputChange(Node& node, Turn& turn);
	void OnTurnPropagate(Turn& turn);

	void OnNodePulse(Node& node, Turn& turn);
	void OnNodeShift(Node& node, Node& oldParent, Node& newParent, Turn& turn);

private:
	void processChildren(Node& node, Turn& turn);
	void invalidateSuccessors(Node& node);

	TopoQueue		scheduledNodes_;
};

} // ~namespace react::topo_sort_impl

using topo_sort_st_impl::TopoSortSTEngine;

} // ~namespace react
