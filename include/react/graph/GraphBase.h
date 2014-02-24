#pragma once

#include <atomic>
#include <memory>
#include <functional>

//#include "react/ReactiveDomain.h"
#include "react/common/Util.h"
#include "react/common/Types.h"

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
template <typename D>
class NodeBase : public std::enable_shared_from_this<NodeBase<D>>
{
public:
	typedef std::shared_ptr<NodeBase>	SharedPtrType;

	SharedPtrType GetSharedPtr() const
	{
		return shared_from_this();
	}
};

////////////////////////////////////////////////////////////////////////////////////////
/// ReactiveNode
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename P,
	typename V
>
class ReactiveNode : public NodeBase<D>, public D::Policy::Engine::NodeInterface
{
public:
	typedef std::shared_ptr<ReactiveNode> NodePtrT;

	typedef D		Domain;
	typedef typename Domain::Policy			Policy;
	typedef typename Domain::Engine			Engine;
	typedef typename Engine::NodeInterface	NodeInterface;
	typedef typename Engine::TurnInterface	TurnInterface;

	explicit ReactiveNode(bool registered) :
		obsCount_(0)
	{
		if (!registered)
			registerNode();
	}

	~ReactiveNode()
	{
		if (GetObsCount() > 0)
			D::Observers().UnregisterFrom(this);

		Engine::OnNodeDestroy(*this);
	}

	virtual const char* GetNodeType() const override { return "ReactiveNode"; }
	
	virtual bool	IsInputNode() const override	{ return false; }
	virtual bool	IsOutputNode() const override	{ return false; }
	virtual bool	IsDynamicNode() const override	{ return false; }

	virtual int		DependencyCount() const		{ return 0; }

	void	IncObsCount()		{ obsCount_.fetch_add(1, std::memory_order_relaxed); }
	void	DecObsCount()		{ obsCount_.fetch_sub(1, std::memory_order_relaxed); }
	uint	GetObsCount() const	{ return obsCount_.load(std::memory_order_relaxed); }

protected:
	void registerNode()
	{
		Engine::OnNodeCreate(*this);
	}

private:
	std::atomic<uint>	obsCount_;
};

template <typename D, typename P, typename V>
using ReactiveNodePtr = typename ReactiveNode<D,P,V>::NodePtrT;

// ---
}