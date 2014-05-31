
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <thread>

#include "react/logging/EventRecords.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// NodeCreateEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
NodeCreateEvent::NodeCreateEvent(ObjectId nodeId, const char* type) :
    nodeId_( nodeId ),
    type_( type )
{}

void NodeCreateEvent::Serialize(std::ostream& out) const
{
    out << "> Node = " << nodeId_ << std::endl;
    out << "> Type = " << type_ << std::endl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// NodeDestroyEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
NodeDestroyEvent::NodeDestroyEvent(ObjectId nodeId) :
    nodeId_( nodeId )
{}

void NodeDestroyEvent::Serialize(std::ostream& out) const
{
    out << "> Node = " << nodeId_ << std::endl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// NodeAttachEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
NodeAttachEvent::NodeAttachEvent(ObjectId nodeId, ObjectId parentId) :
    nodeId_( nodeId ),
    parentId_{ parentId }
{}

void NodeAttachEvent::Serialize(std::ostream& out) const
{
    out << "> Node = " << nodeId_ << std::endl;
    out << "> Parent = " << parentId_ << std::endl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// NodeDetachEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
NodeDetachEvent::NodeDetachEvent(ObjectId nodeId, ObjectId parentId) :
    nodeId_( nodeId ),
    parentId_{ parentId }
{}

void NodeDetachEvent::Serialize(std::ostream& out) const
{
    out << "> Node = " << nodeId_ << std::endl;
    out << "> Parent = " << parentId_ << std::endl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// InputNodeAdmissionEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
InputNodeAdmissionEvent::InputNodeAdmissionEvent(ObjectId nodeId, int transactionId) :
    nodeId_( nodeId ),
    transactionId_( transactionId )
{}

void InputNodeAdmissionEvent::Serialize(std::ostream& out) const
{
    out << "> Node = " << nodeId_ << std::endl;
    out << "> Transaction = " << transactionId_ << std::endl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// NodePulseEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
NodePulseEvent::NodePulseEvent(ObjectId nodeId, int transactionId) :
    nodeId_( nodeId ),
    transactionId_( transactionId )
{}

void NodePulseEvent::Serialize(std::ostream& out) const
{
    out << "> Node = " << nodeId_ << std::endl;
    out << "> Transaction = " << transactionId_ << std::endl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// NodeIdlePulseEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
NodeIdlePulseEvent::NodeIdlePulseEvent(ObjectId nodeId, int transactionId) :
    nodeId_( nodeId ),
    transactionId_( transactionId )
{}

void NodeIdlePulseEvent::Serialize(std::ostream& out) const
{
    out << "> Node = " << nodeId_ << std::endl;
    out << "> Transaction = " << transactionId_ << std::endl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// DynamicNodeAttachEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
DynamicNodeAttachEvent::DynamicNodeAttachEvent(ObjectId nodeId, ObjectId parentId, int transactionId) :
    nodeId_( nodeId ),
    parentId_{ parentId },
    transactionId_( transactionId )
{}

void DynamicNodeAttachEvent::Serialize(std::ostream& out) const
{
    out << "> Node = " << nodeId_ << std::endl;
    out << "> Parent = " << parentId_ << std::endl;
    out << "> Transaction = " << transactionId_ << std::endl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// DynamicNodeDetachEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
DynamicNodeDetachEvent::DynamicNodeDetachEvent(ObjectId nodeId, ObjectId parentId, int transactionId) :
    nodeId_( nodeId ),
    parentId_{ parentId },
    transactionId_( transactionId )
{}

void DynamicNodeDetachEvent::Serialize(std::ostream& out) const
{
    out << "> Node = " << nodeId_ << std::endl;
    out << "> Parent = " << parentId_ << std::endl;
    out << "> Transaction = " << transactionId_ << std::endl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// NodeEvaluateBeginEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
NodeEvaluateBeginEvent::NodeEvaluateBeginEvent(ObjectId nodeId, int transactionId) :
    nodeId_( nodeId ),
    transactionId_( transactionId ),
    threadId_( std::this_thread::get_id() )
{}

void NodeEvaluateBeginEvent::Serialize(std::ostream& out) const
{
    out << "> Node = " << nodeId_ << std::endl;
    out << "> Transaction = " << transactionId_ << std::endl;
    out << "> Thread = " << threadId_ << std::endl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// NodeEvaluateEndEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
NodeEvaluateEndEvent::NodeEvaluateEndEvent(ObjectId nodeId, int transactionId) :
    nodeId_( nodeId ),
    transactionId_( transactionId ),
    threadId_{ std::this_thread::get_id() }
{}

void NodeEvaluateEndEvent::Serialize(std::ostream& out) const
{
    out << "> Node = " << nodeId_ << std::endl;
    out << "> Transaction = " << transactionId_ << std::endl;
    out << "> Thread = " << threadId_ << std::endl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// TransactionBeginEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
TransactionBeginEvent::TransactionBeginEvent(int transactionId) :
    transactionId_( transactionId )
{}

void TransactionBeginEvent::Serialize(std::ostream& out) const
{
    out << "> Transaction = " << transactionId_ << std::endl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// TransactionEndEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
TransactionEndEvent::TransactionEndEvent(int transactionId) :
    transactionId_( transactionId )
{}

void TransactionEndEvent::Serialize(std::ostream& out) const
{
    out << "> Transaction = " << transactionId_ << std::endl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// UserBreakpointEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
UserBreakpointEvent::UserBreakpointEvent(const char* name) :
    name_( name )
{}

void UserBreakpointEvent::Serialize(std::ostream& out) const
{
    out << "> Name = " << name_ << std::endl;
}

/****************************************/ REACT_IMPL_END /***************************************/