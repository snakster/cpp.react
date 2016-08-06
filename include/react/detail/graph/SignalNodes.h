
//          Copyright Sebastian Jeckel 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#ifndef REACT_DETAIL_GRAPH_SIGNALNODES_H_INCLUDED
#define REACT_DETAIL_GRAPH_SIGNALNODES_H_INCLUDED

#include "react/detail/Defs.h"

#include <memory>
#include <utility>

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

    explicit SignalNode(const std::shared_ptr<IReactiveGraph>& graphPtr) :
        SignalNode::NodeBase( graphPtr ),
        value_( )
        { }

    template <typename T>
    SignalNode(const std::shared_ptr<IReactiveGraph>& graphPtr, T&& value) :
        SignalNode::NodeBase( graphPtr ),
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
    explicit VarSignalNode(const std::shared_ptr<IReactiveGraph>& graphPtr) :
        VarSignalNode::SignalNode( graphPtr ),
        newValue_( )
        { this->RegisterMe(); }

    template <typename T>
    VarSignalNode(const std::shared_ptr<IReactiveGraph>& graphPtr, T&& value) :
        VarSignalNode::SignalNode( graphPtr, std::forward<T>(value) ),
        newValue_( value )
        { this->RegisterMe(); }

    ~VarSignalNode()
        { this->UnregisterMe(); }

    virtual const char* GetNodeType() const override
        { return "VarSignal"; }

    virtual bool IsInputNode() const override
        { return true; }

    virtual int GetDependencyCount() const override
        { return 0; }

    virtual UpdateResult Update(TurnId turnId) override
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
    SignalFuncNode(const std::shared_ptr<IReactiveGraph>& graphPtr, FIn&& func, const std::shared_ptr<SignalNode<TDeps>>& ... deps) :
        SignalFuncNode::SignalNode( graphPtr, func(deps->Value() ...) ),
        func_( std::forward<FIn>(func) ),
        depHolder_( deps ... )
    {
        this->RegisterMe();
        REACT_EXPAND_PACK(this->AttachToMe(deps->GetNodeId()));
    }

    ~SignalFuncNode()
    {
        apply([this] (const auto& ... deps) { REACT_EXPAND_PACK(this->DetachFromMe(deps->GetNodeId())); }, depHolder_);
        this->UnregisterMe();
    }

    virtual const char* GetNodeType() const override
        { return "SignalFunc"; }

    virtual int GetDependencyCount() const override
        { return sizeof...(TDeps); }

    virtual UpdateResult Update(TurnId turnId) override
    {   
        bool changed = false;

        S newValue = apply([this] (const auto& ... deps) { return this->func_(deps->Value() ...); }, depHolder_);

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

    std::tuple<std::shared_ptr<SignalNode<TDeps>> ...> depHolder_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// FlattenNode
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TOuter, typename TInner>
class SignalFlattenNode : public SignalNode<TInner>
{
public:
    SignalFlattenNode(const std::shared_ptr<IReactiveGraph>& graphPtr, const std::shared_ptr<SignalNode<TOuter>>& outer, const std::shared_ptr<SignalNode<TInner>>& inner) :
        SignalFlattenNode::SignalNode( graphPtr, inner->Value() ),
        outer_( outer ),
        inner_( inner )
    {
        this->RegisterMe();
        this->AttachToMe(outer->GetNodeId());
        this->AttachToMe(inner->GetNodeId());
    }

    ~SignalFlattenNode()
    {
        this->DetachFromMe(inner->GetNodeId());
        this->DetachFromMe(outer->GetNodeId());
        this->UnregisterMe();
    }

    virtual const char* GetNodeType() const override
        { return "SignalFlatten"; }

    virtual bool IsDynamicNode() const override
        { return true; }

    virtual int GetDependencyCount() const override
        { return 2; }

    virtual UpdateResult Update(TurnId turnId) override
    {
        auto newInner = GetNodePtr(outer_->Value());

        if (newInner != inner_)
        {
            // Topology has been changed
            auto oldInner = inner_;
            inner_ = newInner;

            this->DynamicDetachFromMe(oldInner->GetNodeId(), 0);
            this->DynamicAttachToMe(newInner->GetNodeId(), 0);

            return UpdateResult::shifted;
        }

        if (! Equals(this->Value(), inner_->Value()))
        {
            this->Value() = inner_->Value();
            return UpdateResult::changed;
        }
        else
        {
            return UpdateResult::unchanged;
        }
    }

private:
    std::shared_ptr<SignalNode<TOuter>> outer_;
    std::shared_ptr<SignalNode<TInner>> inner_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_GRAPH_SIGNALNODES_H_INCLUDED