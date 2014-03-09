#pragma once

#include "tbb/queuing_rw_mutex.h"

#include "EngineBase.h"
#include "react/ReactiveDomain.h"
#include "react/common/Containers.h"
#include "react/common/TopoQueue.h"
#include "react/common/Types.h"

////////////////////////////////////////////////////////////////////////////////////////
REACT_IMPL_BEGIN
namespace toposort_st {

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
class Turn : public TurnBase
{
public:
	Turn(TurnIdT id, TurnFlagsT flags);
};

////////////////////////////////////////////////////////////////////////////////////////
/// EngineBase
////////////////////////////////////////////////////////////////////////////////////////
template <typename TTurn>
class EngineBase : public IReactiveEngine<Node,TTurn>
{
public:
	typedef TopoQueue<Node>	TopoQueue;

	void OnNodeAttach(Node& node, Node& parent);
	void OnNodeDetach(Node& node, Node& parent);

	void OnTurnInputChange(Node& node, TTurn& turn);
	void OnTurnPropagate(Turn& turn);

	void OnNodePulse(Node& node, TTurn& turn);
	void OnNodeShift(Node& node, Node& oldParent, Node& newParent, TTurn& turn);

private:
	void processChildren(Node& node, TTurn& turn);
	void invalidateSuccessors(Node& node);

	TopoQueue		scheduledNodes_;
};

class BasicEngine :	public EngineBase<Turn> {};
class QueuingEngine : public DefaultQueuingEngine<EngineBase,Turn> {};

} // ~namespace toposort_st
REACT_IMPL_END

REACT_BEGIN

struct sequential;
struct sequential_queuing;

template <typename TMode>
class TopoSortEngine;

template <> class TopoSortEngine<sequential> : public REACT_IMPL::toposort_st::BasicEngine {};
template <> class TopoSortEngine<sequential_queuing> : public REACT_IMPL::toposort_st::QueuingEngine {};

REACT_END
