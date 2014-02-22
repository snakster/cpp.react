#pragma once

#include <memory>
#include <functional>

//#include "react/ReactiveDomain.h"
#include "react/common/Util.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace react {

////////////////////////////////////////////////////////////////////////////////////////
/// TickResult
////////////////////////////////////////////////////////////////////////////////////////
enum class ETickResult
{
	none,
	pulsed,
	idle_pulsed,
	invalidated
};

////////////////////////////////////////////////////////////////////////////////////////
/// IReactiveNode
////////////////////////////////////////////////////////////////////////////////////////
struct IReactiveNode
{
	virtual ~IReactiveNode()	{}

	// Unique type identifier
	virtual const char* GetNodeType() const = 0;

	virtual ETickResult	Tick(void* turnPtr) = 0;

	/// Input nodes can be manipulated externally.
	virtual bool	IsInputNode() const = 0;

	/// Output nodes can't have any successors.
	virtual bool	IsOutputNode() const = 0;

	/// This node can have successors and may be re-attached to other nodes.
	virtual bool	IsDynamicNode() const = 0;

	virtual int		DependencyCount() const = 0;
};

////////////////////////////////////////////////////////////////////////////////////////
/// NodeBase
////////////////////////////////////////////////////////////////////////////////////////
template <typename TDomain>
class NodeBase : public std::enable_shared_from_this<NodeBase<TDomain>>
{
public:
	typedef std::shared_ptr<NodeBase>	SharedPtrType;
	typedef std::weak_ptr<NodeBase>		WeakPtrType;

	SharedPtrType GetSharedPtr()
	{
		return shared_from_this();
	}
};

////////////////////////////////////////////////////////////////////////////////////////
/// ReactiveNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename TDomain,
	typename P,
	typename V
>
class ReactiveNode : public NodeBase<TDomain>, public TDomain::Policy::Engine::NodeInterface
{
public:
	typedef std::shared_ptr<ReactiveNode> NodePtrT;

	typedef TDomain					Domain;
	typedef typename Domain::Policy			Policy;
	typedef typename Domain::Engine			Engine;
	typedef typename Engine::NodeInterface	NodeInterface;
	typedef typename Engine::TurnInterface	TurnInterface;

	explicit ReactiveNode(bool registered)
	{
		if (!registered)
			registerNode();
	}

	~ReactiveNode()
	{
		Engine::OnNodeDestroy(*this);
	}

	virtual const char* GetNodeType() const override { return "ReactiveNode"; }
	
	virtual bool	IsInputNode() const override	{ return false; }
	virtual bool	IsOutputNode() const override	{ return false; }
	virtual bool	IsDynamicNode() const override	{ return false; }

	virtual int		DependencyCount() const		{ return 0; }

protected:
	void registerNode()
	{
		Engine::OnNodeCreate(*this);
	}
};

template <typename TDomain, typename P, typename V>
using ReactiveNodePtr = typename ReactiveNode<TDomain,P,V>::NodePtrT;

// ---
}