
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/Defs.h"

/*********************************/ REACT_IMPL_BEGIN /*********************************/

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
	using NodeInterface = TNodeInterface;
	using TurnInterface = TTurnInterface;

	void OnNodeCreate(NodeInterface& node)								{}
	void OnNodeDestroy(NodeInterface& node)								{}

	void OnNodeAttach(NodeInterface& node, NodeInterface& parent)		{}
	void OnNodeDetach(NodeInterface& node, NodeInterface& parent)		{}

	void OnTurnAdmissionStart(TurnInterface& turn)						{}
	void OnTurnAdmissionEnd(TurnInterface& turn)						{}
	void OnTurnEnd(TurnInterface& turn)									{}

	void OnTurnInputChange(NodeInterface& node, TurnInterface& turn)	{}
	void OnTurnPropagate(TurnInterface& turn)							{}

	void OnNodePulse(NodeInterface& node, TurnInterface& turn)			{}
	void OnNodeIdlePulse(NodeInterface& node, TurnInterface& turn)		{}

	//void OnNodeShift(NodeInterface& node, NodeInterface& oldParent, NodeInterface& newParent, TurnInterface& turn)	{}

	void OnDynamicNodeAttach(NodeInterface& node, NodeInterface& parent, TurnInterface& turn)	{}
	void OnDynamicNodeDetach(NodeInterface& node, NodeInterface& parent, TurnInterface& turn)	{}

	template <typename F>
	bool TryMerge(F&& f) { return false; }
};

////////////////////////////////////////////////////////////////////////////////////////
/// EngineInterface - Static wrapper for IReactiveEngine
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename TEngine
>
struct EngineInterface
{
	using NodeInterface = typename TEngine::NodeInterface;
	using TurnInterface = typename TEngine::TurnInterface;

	static TEngine& Engine()
	{
		static TEngine engine;
		return engine;
	}

	static void OnNodeCreate(NodeInterface& node)
	{
		D::Log().template Append<NodeCreateEvent>(GetObjectId(node), node.GetNodeType());
		Engine().OnNodeCreate(node);
	}

	static void OnNodeDestroy(NodeInterface& node)
	{
		D::Log().template Append<NodeDestroyEvent>(GetObjectId(node));
		Engine().OnNodeDestroy(node);
	}

	static void OnNodeAttach(NodeInterface& node, NodeInterface& parent)
	{
		D::Log().template Append<NodeAttachEvent>(GetObjectId(node), GetObjectId(parent));
		Engine().OnNodeAttach(node, parent);
	}

	static void OnNodeDetach(NodeInterface& node, NodeInterface& parent)
	{
		D::Log().template Append<NodeDetachEvent>(GetObjectId(node), GetObjectId(parent));
		Engine().OnNodeDetach(node, parent);
	}

	static void OnNodePulse(NodeInterface& node, TurnInterface& turn)
	{
		D::Log().template Append<NodePulseEvent>(GetObjectId(node), turn.Id());
		Engine().OnNodePulse(node, turn);
	}

	static void OnNodeIdlePulse(NodeInterface& node, TurnInterface& turn)
	{
		D::Log().template Append<NodeIdlePulseEvent>(GetObjectId(node), turn.Id());
		Engine().OnNodeIdlePulse(node, turn);
	}

	static void OnDynamicNodeAttach(NodeInterface& node, NodeInterface& parent, TurnInterface& turn)
	{
		Engine().OnDynamicNodeAttach(node, parent, turn);
	}

	static void OnDynamicNodeDetach(NodeInterface& node, NodeInterface& parent, TurnInterface& turn)
	{
		Engine().OnDynamicNodeDetach(node, parent, turn);
	}

	static void OnTurnAdmissionStart(TurnInterface& turn)
	{
		D::Log().template Append<TransactionBeginEvent>(turn.Id());
		Engine().OnTurnAdmissionStart(turn);
	}

	static void OnTurnAdmissionEnd(TurnInterface& turn)
	{
		Engine().OnTurnAdmissionEnd(turn);
	}

	static void OnTurnEnd(TurnInterface& turn)
	{
		D::Log().template Append<TransactionEndEvent>(turn.Id());
		Engine().OnTurnEnd(turn);
	}

	static void OnTurnInputChange(NodeInterface& node, TurnInterface& turn)
	{
		D::Log().template Append<InputNodeAdmissionEvent>(GetObjectId(node), turn.Id());
		Engine().OnTurnInputChange(node, turn);
	}

	static void OnTurnPropagate(TurnInterface& turn)
	{
		Engine().OnTurnPropagate(turn);
	}

	template <typename F>
	static bool TryMerge(F&& f)
	{
		return Engine().TryMerge(std::forward<F>(f));
	}
};

/**********************************/ REACT_IMPL_END /**********************************/