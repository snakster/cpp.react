
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_DETAIL_LOGGING_EVENTRECORDS_H_INCLUDED
#define REACT_DETAIL_LOGGING_EVENTRECORDS_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <iostream>
#include <string>
#include <thread>

#include "Logging.h"
#include "react/common/Types.h"

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// NodeCreateEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
class NodeCreateEvent : public IEventRecord
{
public:
    NodeCreateEvent(ObjectId nodeId, const char* type);

    virtual const char* EventId() const { return "NodeCreate"; }

    virtual void Serialize(std::ostream& out) const;

private:
    ObjectId        nodeId_;
    const char *    type_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// NodeDestroyEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
class NodeDestroyEvent : public IEventRecord
{
public:
    NodeDestroyEvent(ObjectId nodeId);

    virtual const char* EventId() const { return "NodeDestroy"; }

    virtual void Serialize(std::ostream& out) const;

private:
    ObjectId    nodeId_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// NodeAttachEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
class NodeAttachEvent : public IEventRecord
{
public:
    NodeAttachEvent(ObjectId nodeId, ObjectId parentId);

    virtual const char* EventId() const { return "NodeAttach"; }
    
    virtual void Serialize(std::ostream& out) const;

private:
    ObjectId    nodeId_;
    ObjectId    parentId_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// NodeDetachEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
class NodeDetachEvent : public IEventRecord
{
public:
    NodeDetachEvent(ObjectId nodeId, ObjectId parentId);

    virtual const char* EventId() const { return "NodeDetach"; }
    
    virtual void Serialize(std::ostream& out) const;

private:
    ObjectId    nodeId_;
    ObjectId    parentId_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// InputNodeAdmissionEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
class InputNodeAdmissionEvent : public IEventRecord
{
public:
    InputNodeAdmissionEvent(ObjectId nodeId, int transactionId);

    virtual const char* EventId() const { return "InputNodeAdmission"; }

    virtual void Serialize(std::ostream& out) const;

private:
    ObjectId    nodeId_;
    int         transactionId_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// NodePulseEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
class NodePulseEvent : public IEventRecord
{
public:
    NodePulseEvent(ObjectId nodeId, int transactionId);

    virtual const char* EventId() const { return "NodePulse"; }

    virtual void Serialize(std::ostream& out) const;

private:
    ObjectId    nodeId_;
    int         transactionId_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// NodeIdlePulseEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
class NodeIdlePulseEvent : public IEventRecord
{
public:
    NodeIdlePulseEvent(ObjectId nodeId, int transactionId);

    virtual const char* EventId() const { return "NodeIdlePulse"; }

    virtual void Serialize(std::ostream& out) const;

private:
    ObjectId    nodeId_;
    int         transactionId_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// DynamicNodeAttachEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
class DynamicNodeAttachEvent : public IEventRecord
{
public:
    DynamicNodeAttachEvent(ObjectId nodeId, ObjectId parentId, int transactionId);

    virtual const char* EventId() const { return "DynamicNodeAttach"; }
    
    virtual void Serialize(std::ostream& out) const;

private:
    ObjectId    nodeId_;
    ObjectId    parentId_;
    int         transactionId_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// DynamicNodeDetachEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
class DynamicNodeDetachEvent : public IEventRecord
{
public:
    DynamicNodeDetachEvent(ObjectId nodeId, ObjectId parentId, int transactionId);

    virtual const char* EventId() const { return "DynamicNodeDetach"; }
    
    virtual void Serialize(std::ostream& out) const;

private:
    ObjectId    nodeId_;
    ObjectId    parentId_;
    int         transactionId_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// NodeEvaluateBeginEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
class NodeEvaluateBeginEvent : public IEventRecord
{
public:
    NodeEvaluateBeginEvent(ObjectId nodeId, int transactionId);

    virtual const char* EventId() const { return "NodeEvaluateBegin"; }

    virtual void Serialize(std::ostream& out) const;

private:
    ObjectId        nodeId_;
    int             transactionId_;
    std::thread::id threadId_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// NodeEvaluateEndEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
class NodeEvaluateEndEvent : public IEventRecord
{
public:
    NodeEvaluateEndEvent(ObjectId nodeId, int transactionId);

    virtual const char* EventId() const { return "NodeEvaluateEnd"; }

    virtual void Serialize(std::ostream& out) const;

private:
    ObjectId        nodeId_;
    int             transactionId_;
    std::thread::id threadId_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// TransactionBeginEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
class TransactionBeginEvent : public IEventRecord
{
public:
    TransactionBeginEvent(int transactionId);

    virtual const char* EventId() const { return "TransactionBegin"; }

    virtual void Serialize(std::ostream& out) const;

private:
    int transactionId_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// TransactionEndEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
class TransactionEndEvent : public IEventRecord
{
public:
    TransactionEndEvent(int transactionId);

    virtual const char* EventId() const { return "TransactionEnd"; }

    virtual void Serialize(std::ostream& out) const;

private:
    int transactionId_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// UserBreakpointEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
class UserBreakpointEvent : public IEventRecord
{
public:
    UserBreakpointEvent(const char* name);

    virtual const char* EventId() const { return "UserBreakpoint"; }

    virtual void Serialize(std::ostream& out) const;

private:
    std::string name_;
};

/****************************************/ REACT_IMPL_END /***************************************/

#endif // REACT_DETAIL_LOGGING_EVENTRECORDS_H_INCLUDED