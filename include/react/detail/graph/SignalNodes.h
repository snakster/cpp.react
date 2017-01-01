
//          Copyright Sebastian Jeckel 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#ifndef REACT_DETAIL_GRAPH_SIGNALNODES_H_INCLUDED
#define REACT_DETAIL_GRAPH_SIGNALNODES_H_INCLUDED

#include "react/detail/Defs.h"

#include <memory>
#include <queue>
#include <utility>
#include <vector>

#include "GraphBase.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward declarations
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename L, typename R>
bool Equals(const L& lhs, const R& rhs);

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SignalNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class SignalNode : public NodeBase
{
public:
    SignalNode(SignalNode&&) = default;
    SignalNode& operator=(SignalNode&&) = default;

    SignalNode(const SignalNode&) = delete;
    SignalNode& operator=(const SignalNode&) = delete;

    explicit SignalNode(const Group& group) :
        SignalNode::NodeBase( group ),
        value_( )
        { }

    template <typename T>
    SignalNode(const Group& group, T&& value) :
        SignalNode::NodeBase( group ),
        value_( std::forward<T>(value) )
        { }

    S& Value()
        { return value_; }

    const S& Value() const
        { return value_; }

private:
    S value_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// VarNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class VarSignalNode : public SignalNode<S>
{
public:
    explicit VarSignalNode(const Group& group) :
        VarSignalNode::SignalNode( group ),
        newValue_( )
    {
        this->RegisterMe(NodeCategory::input);
    }

    template <typename T>
    VarSignalNode(const Group& group, T&& value) :
        VarSignalNode::SignalNode( group, std::forward<T>(value) ),
        newValue_( value )
    {
        this->RegisterMe();
    }

    ~VarSignalNode()
    {
        this->UnregisterMe();
    }

    virtual const char* GetNodeType() const override
        { return "VarSignal"; }

    virtual int GetDependencyCount() const override
        { return 0; }

    virtual UpdateResult Update(TurnId turnId, size_t successorCount) override
    {
        if (isInputAdded_)
        {
            isInputAdded_ = false;

            if (! (this->Value() == newValue_))
            {
                this->Value() = std::move(newValue_);
                return UpdateResult::changed;
            }
            else
            {
                return UpdateResult::unchanged;
            }
        }
        else if (isInputModified_)
        {            
            isInputModified_ = false;
            return UpdateResult::changed;
        }

        else
        {
            return UpdateResult::unchanged;
        }
    }

    template <typename T>
    void SetValue(T&& newValue)
    {
        newValue_ = std::forward<T>(newValue);

        isInputAdded_ = true;

        // isInputAdded_ takes precedences over isInputModified_
        // the only difference between the two is that isInputModified_ doesn't/can't compare
        isInputModified_ = false;
    }

    // This is signal-specific
    template <typename F>
    void ModifyValue(F&& func)
    {
        // There hasn't been any Set(...) input yet, modify.
        if (! isInputAdded_)
        {
            func(this->Value());

            isInputModified_ = true;
        }
        // There's a newValue, modify newValue instead.
        // The modified newValue will handled like before, i.e. it'll be compared to value_
        // in ApplyInput
        else
        {
            func(newValue_);            
        }
    }

private:
    S       newValue_;
    bool    isInputAdded_ = false;
    bool    isInputModified_ = false;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SignalOpNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S, typename F, typename ... TDeps>
class SignalFuncNode : public SignalNode<S>
{
public:
    template <typename FIn>
    SignalFuncNode(const Group& group, FIn&& func, const Signal<TDeps>& ... deps) :
        SignalFuncNode::SignalNode( group, func(deps->Value() ...) ),
        func_( std::forward<FIn>(func) ),
        depHolder_( deps ... )
    {
        this->RegisterMe();
        REACT_EXPAND_PACK(this->AttachToMe(deps->GetNodeId()));
    }

    ~SignalFuncNode()
    {
        apply([this] (const auto& ... deps) { REACT_EXPAND_PACK(this->DetachFromMe(GetInternals(deps).GetNodePtr()->GetNodeId())); }, depHolder_);
        this->UnregisterMe();
    }

    virtual const char* GetNodeType() const override
        { return "SignalFunc"; }

    virtual int GetDependencyCount() const override
        { return sizeof...(TDeps); }

    virtual UpdateResult Update(TurnId turnId, size_t successorCount) override
    {   
        bool changed = false;

        S newValue = apply([this] (const auto& ... deps) { return this->func_(GetInternals(deps).Value() ...); }, depHolder_);

        if (! (this->Value() == newValue))
        {
            this->Value() = std::move(newValue);
            changed = true;
        }

        if (changed)
            return UpdateResult::changed;
        else
            return UpdateResult::unchanged;
    }

private:
    F func_;

    std::tuple<Signal<TDeps> ...> depHolder_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SignalSlotNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class SignalSlotNode : public SignalNode<S>
{
public:
    SignalSlotNode(const Group& group, const Signal<S>& dep) :
        SignalSlotNode::SignalNode( group, dep->Value() ),
        slotInput_( *this, dep )
    {
        slotInput_.nodeId = GraphPtr()->RegisterNode(&slotInput_, NodeCategory::dyninput);
        this->RegisterMe();

        this->AttachToMe(slotInput_.nodeId);
        this->AttachToMe(dep->GetNodeId());
    }

    ~SignalSlotNode()
    {
        const auto& depNodePtr = GetInternals(slotInput_.dep).GetNodePtr();

        this->DetachFromMe(depNodePtr->GetNodeId());
        this->DetachFromMe(slotInput_.nodeId);

        this->UnregisterMe();
        GetGraphPtr()->UnregisterNode(slotInput_.nodeId);
    }

    virtual const char* GetNodeType() const override
        { return "SignalSlot"; }

    virtual int GetDependencyCount() const override
        { return 2; }

    virtual UpdateResult Update(TurnId turnId, size_t successorCount) override
    {
        const auto& depNodePtr = GetInternals(slotInput_.dep).GetNodePtr();

        if (! (this->Value() == depNodePtr->Value()))
        {
            this->Value() = depNodePtr->Value();
            return UpdateResult::changed;
        }
        else
        {
            return UpdateResult::unchanged;
        }
    }

    void SetInput(const Signal<S>& newInput)
        { slotInput_.newDep = newInput; }

    NodeId GetInputNodeId() const
        { return slotInput_.nodeId; }

private:        
    struct VirtualInputNode : public IReactiveNode
    {
        VirtualInputNode(SignalSlotNode& parentIn, const Signal<S>& depIn) :
            parent( parentIn ),
            dep( depIn ),
            newDep( depIn ),
            { }

        virtual const char* GetNodeType() const override
            { return "SignalSlotInput"; }

        virtual int GetDependencyCount() const override
            { return 0; }

        virtual UpdateResult Update(TurnId turnId, size_t successorCount) override
        {
            if (dep != newDep)
            {
                const auto& depNodePtr = GetInternals(dep).GetNodePtr();
                const auto& newDepNodePtr = GetInternals(newDep).GetNodePtr();

                parent.DynamicDetachFromMe(depNodePtr->GetNodeId(), 0);
                parent.DynamicAttachToMe(newDepNodePtr->GetNodeId(), 0);

                dep = std::move(newDep);
                return UpdateResult::changed;
            }
            else
            {
                return UpdateResult::unchanged;
            }
        }

        SignalSlotNode& parent;

        NodeId nodeId;

        Signal<S> dep;
        Signal<S> newDep;
    };

    VirtualInputNode slotInput_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SignalLinkNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class SignalLinkNode : public SignalNode<S>
{
public:
    SignalLinkNode(const Group& group, const Group& srcGroup, const Signal<S>& dep) :
        SignalLinkNode::SignalNode( group, dep->Value() ),
        linkOutput_( srcGroup, dep )
    {
        this->RegisterMe(NodeCategory::input);
    }

    ~SignalLinkNode()
    {
        this->UnregisterMe();
    }

    void SetWeakSelfPtr(const std::weak_ptr<SignalLinkNode>& self)
        { linkOutput_.parent = self; }

    virtual const char* GetNodeType() const override
        { return "SignalLink"; }

    virtual int GetDependencyCount() const override
        { return 1; }

    virtual UpdateResult Update(TurnId turnId, size_t successorCount) override
        { return UpdateResult::changed; }

    template <typename T>
    void SetValue(T&& newValue)
        { this->Value() = std::forward<T>(newValue); }

private:
    struct VirtualOutputNode : public ILinkOutputNode
    {
        VirtualOutputNode(const Group& srcGroupIn, const Signal<S>& depIn) :
            parent( ),
            srcGroup( srcGroupIn ),
            dep( depIn )
        {
            nodeId = srcGraphPtr->RegisterNode(this, NodeCategory::linkoutput);
            srcGraphPtr->OnNodeAttach(nodeId, dep->GetNodeId());
        }

        ~VirtualOutputNode()
        {
            srcGraphPtr->OnNodeDetach(nodeId, dep->GetNodeId());
            srcGraphPtr->UnregisterNode(nodeId);
        }

        virtual const char* GetNodeType() const override
            { return "SignalLinkOutput"; }

        virtual int GetDependencyCount() const override
            { return 1; }

        virtual UpdateResult Update(TurnId turnId, size_t successorCount) override
            { return UpdateResult::changed; }

        virtual void CollectOutput(LinkOutputMap& output) override
        {
            if (auto p = parent.lock())
            {
                auto* rawPtr = p->GraphPtr().get();
                output[rawPtr].push_back(
                    [storedParent = std::move(p), storedValue = dep->Value()]
                    {
                        NodeId nodeId = storedParent->GetNodeId();
                        auto& graphPtr = storedParent->GraphPtr();

                        graphPtr->AddInput(nodeId,
                            [&storedParent, &storedValue]
                            {
                                storedParent->SetValue(std::move(storedValue));
                            });
                    });
            }
        }

        std::weak_ptr<SignalLinkNode> parent;

        NodeId nodeId;

        Signal<S> dep;

        Group srcGroup;
    };

    VirtualOutputNode linkOutput_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_GRAPH_SIGNALNODES_H_INCLUDED