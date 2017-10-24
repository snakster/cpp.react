
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#ifndef REACT_DETAIL_GRAPH_SIGNALNODES_H_INCLUDED
#define REACT_DETAIL_GRAPH_SIGNALNODES_H_INCLUDED

#include "react/detail/defs.h"

#include <memory>
#include <queue>
#include <utility>
#include <vector>

#include "node_base.h"

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

    virtual UpdateResult Update(TurnId turnId) noexcept override
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
        SignalFuncNode::SignalNode( group, func(GetInternals(deps).Value() ...) ),
        func_( std::forward<FIn>(func) ),
        depHolder_( deps ... )
    {
        this->RegisterMe();
        REACT_EXPAND_PACK(this->AttachToMe(GetInternals(deps).GetNodeId()));
    }

    ~SignalFuncNode()
    {
        std::apply([this] (const auto& ... deps)
            { REACT_EXPAND_PACK(this->DetachFromMe(GetInternals(deps).GetNodePtr()->GetNodeId())); }, depHolder_);
        this->UnregisterMe();
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {   
        bool changed = false;

        S newValue = std::apply([this] (const auto& ... deps)
            { return this->func_(GetInternals(deps).Value() ...); }, depHolder_);

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
        SignalSlotNode::SignalNode( group, GetInternals(dep).Value() ),
        input_( dep )
    {
        inputNodeId_ = GetGraphPtr()->RegisterNode(&slotInput_, NodeCategory::dyninput);
        
        this->RegisterMe();
        this->AttachToMe(inputNodeId_);
        this->AttachToMe(GetInternals(dep).GetNodeId());
    }

    ~SignalSlotNode()
    {
        this->DetachFromMe(GetInternals(input_).GetNodeId());
        this->DetachFromMe(inputNodeId_);
        this->UnregisterMe();

        GetGraphPtr()->UnregisterNode(inputNodeId_);
    }

    virtual UpdateResult Update(TurnId turnId) noexcept override
    {
        if (! (this->Value() == GetInternals(input_).Value()))
        {
            this->Value() = GetInternals(input_).Value();
            return UpdateResult::changed;
        }
        else
        {
            return UpdateResult::unchanged;
        }
    }

    void SetInput(const Signal<S>& newInput)
    {
        if (newInput == input_)
            return;

        this->DetachFromMe(GetInternals(input_).GetNodeId());
        this->AttachToMe(GetInternals(newInput).GetNodeId());

        input_ = newInput;
    }

    NodeId GetInputNodeId() const
        { return inputNodeId_; }

private:        
    struct VirtualInputNode : public IReactNode
    {
        virtual UpdateResult Update(TurnId turnId) noexcept override
            { return UpdateResult::changed; }
    };

    Signal<S>           input_;
    NodeId              inputNodeId_;
    VirtualInputNode    slotInput_;
    
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SignalLinkNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class SignalLinkNode : public SignalNode<S>
{
public:
    SignalLinkNode(const Group& group, const Signal<S>& dep) :
        SignalLinkNode::SignalNode( group, GetInternals(dep).Value() ),
        dep_ ( dep )
    {
        this->RegisterMe(NodeCategory::input);

        auto& srcGraphPtr = GetInternals(srcGroup).GetGraphPtr();
        outputNodeId_ = srcGraphPtr->RegisterNode(this, NodeCategory::linkoutput);
        
        srcGraphPtr->AttachNode(outputNodeId_, GetInternals(dep).GetNodeId());
    }

    ~SignalLinkNode()
    {
        this->DetachFromMe(GetInternals(dep_).GetNodeId());

        auto& srcGraphPtr = GetInternals(srcGroup).GetGraphPtr();
        srcGraphPtr->DetachNode(outputNodeId_, GetInternals(dep_).GetNodeId());
        srcGraphPtr->UnregisterNode(outputNodeId_);

        auto& linkCache = GetGraphPtr()->GetLinkCache();
        linkCache.Erase(this);

        this->UnregisterMe();
    }

    void SetWeakSelfPtr(const std::weak_ptr<SignalLinkNode>& self)
        { linkOutput_.parent = self; }

    virtual UpdateResult Update(TurnId turnId) noexcept override
        { return UpdateResult::changed; }

    void SetValue(S&& newValue)
        { this->Value() = std::move(newValue); }

private:
    struct VirtualOutputNode : public IReactNode
    {
        virtual UpdateResult Update(TurnId turnId) noexcept override
            { return UpdateResult::changed; }

        virtual void CollectOutput(LinkOutputMap& output) override
        {
            if (auto p = parent.lock())
            {
                auto* rawPtr = p->GetGraphPtr().get();
                output[rawPtr].push_back(
                    [storedParent = std::move(p), storedValue = GetInternals(p->dep_).Value()] () mutable -> void
                    {
                        NodeId nodeId = storedParent->GetNodeId();
                        auto& graphPtr = storedParent->GetGraphPtr();

                        graphPtr->AddInput(nodeId,
                            [&storedParent, &storedValue]
                            {
                                storedParent->SetValue(std::move(storedValue));
                            });
                    });
            }
        }

        std::weak_ptr<SignalLinkNode> parent;
    };

    Signal<S>   dep_;
    Group       srcGroup;
    NodeId      outputNodeId_;
    VirtualOutputNode linkOutput_;
};


///////////////////////////////////////////////////////////////////////////////////////////////////
/// SignalInternals
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename S>
class SignalInternals
{
public:
    SignalInternals(const SignalInternals&) = default;
    SignalInternals& operator=(const SignalInternals&) = default;

    SignalInternals(SignalInternals&&) = default;
    SignalInternals& operator=(SignalInternals&&) = default;

    explicit SignalInternals(std::shared_ptr<SignalNode<S>>&& nodePtr) :
        nodePtr_( std::move(nodePtr) )
    { }

    auto GetNodePtr() -> std::shared_ptr<SignalNode<S>>&
        { return nodePtr_; }

    auto GetNodePtr() const -> const std::shared_ptr<SignalNode<S>>&
        { return nodePtr_; }

    NodeId GetNodeId() const
        { return nodePtr_->GetNodeId(); }

    S& Value()
        { return nodePtr_->Value(); }

    const S& Value() const
        { return nodePtr_->Value(); }

private:
    std::shared_ptr<SignalNode<S>> nodePtr_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_GRAPH_SIGNALNODES_H_INCLUDED