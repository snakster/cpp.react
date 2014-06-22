
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MoveTest.h"
#include "TestUtil.h"

#include "react/engine/ToposortEngine.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;

using P1 = DomainParams<sequential,ToposortEngine>;

INSTANTIATE_TYPED_TEST_CASE_P(SeqToposort, MoveTest, P1);

} // ~namespace