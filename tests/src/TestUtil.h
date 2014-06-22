
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#ifndef REACT_TESTS_TESTUTIL_H_INCLUDED
#define REACT_TESTS_TESTUTIL_H_INCLUDED

template
<
    EDomainMode m,
    template <EPropagationMode> class TTEngine
>
struct DomainParams
{
    static const EDomainMode mode = m;

    template <EPropagationMode pm>
    using EngineT = TTEngine<pm>;
};

#endif // REACT_TESTS_TESTUTIL_H_INCLUDED