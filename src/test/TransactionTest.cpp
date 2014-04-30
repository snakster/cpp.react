
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "TransactionTest.h"

#include "react/engine/PulseCountEngine.h"
#include "react/engine/ToposortEngine.h"
#include "react/engine/SubtreeEngine.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;

INSTANTIATE_TYPED_TEST_CASE_P(SeqToposort, TransactionTest, ToposortEngine<sequential_queue>);
INSTANTIATE_TYPED_TEST_CASE_P(ParToposort, TransactionTest, ToposortEngine<parallel_queue>);
INSTANTIATE_TYPED_TEST_CASE_P(PulseCount, TransactionTest, PulseCountEngine<parallel_queue>);
INSTANTIATE_TYPED_TEST_CASE_P(ParToposortP, TransactionTest, ToposortEngine<parallel_pipeline>);
INSTANTIATE_TYPED_TEST_CASE_P(Subtree, TransactionTest, SubtreeEngine<parallel_queue>);

} // ~namespace