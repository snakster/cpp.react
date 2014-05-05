
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "EventStreamTest.h"

#include "react/engine/PulsecountEngine.h"
#include "react/engine/ToposortEngine.h"
#include "react/engine/SubtreeEngine.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;

INSTANTIATE_TYPED_TEST_CASE_P(SeqToposortQ, EventStreamTest, ToposortEngine<sequential_queue>);
INSTANTIATE_TYPED_TEST_CASE_P(ParToposortQ, EventStreamTest, ToposortEngine<parallel_queue>);
INSTANTIATE_TYPED_TEST_CASE_P(PulsecountQ, EventStreamTest, PulsecountEngine<parallel_queue>);
INSTANTIATE_TYPED_TEST_CASE_P(ParToposortP, EventStreamTest, ToposortEngine<parallel_pipeline>);
INSTANTIATE_TYPED_TEST_CASE_P(SubtreeQ, EventStreamTest, SubtreeEngine<parallel_queue>);

} // ~namespace