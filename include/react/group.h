
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_GROUP_H_INCLUDED
#define REACT_GROUP_H_INCLUDED

#pragma once

#include "react/detail/defs.h"

#include <memory>
#include <utility>

#include "react/API.h"
#include "react/common/syncpoint.h"

#include "react/detail/graph_interface.h"
#include "react/detail/graph_impl.h"

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Group
///////////////////////////////////////////////////////////////////////////////////////////////////
class Group : protected REACT_IMPL::GroupInternals
{
public:
    Group() = default;

    Group(const Group&) = default;
    Group& operator=(const Group&) = default;

    Group(Group&&) = default;
    Group& operator=(Group&&) = default;

    template <typename F>
    void DoTransaction(F&& func)
        { GetGraphPtr()->DoTransaction(std::forward<F>(func)); }

    template <typename F>
    void EnqueueTransaction(F&& func, TransactionFlags flags = TransactionFlags::none)
        { GetGraphPtr()->EnqueueTransaction(std::forward<F>(func), SyncPoint::Dependency{ }, flags); }

    template <typename F>
    void EnqueueTransaction(F&& func, const SyncPoint& syncPoint, TransactionFlags flags = TransactionFlags::none)
        { GetGraphPtr()->EnqueueTransaction(std::forward<F>(func), SyncPoint::Dependency{ syncPoint }, flags); }

    friend bool operator==(const Group& a, const Group& b)
        { return a.GetGraphPtr() == b.GetGraphPtr(); }

    friend bool operator!=(const Group& a, const Group& b)
        { return !(a == b); }

    friend auto GetInternals(Group& g) -> REACT_IMPL::GroupInternals&
        { return g; }

    friend auto GetInternals(const Group& g) -> const REACT_IMPL::GroupInternals&
        { return g; }
};

/******************************************/ REACT_END /******************************************/

#endif // REACT_GROUP_H_INCLUDED