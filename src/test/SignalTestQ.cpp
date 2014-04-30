
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "SignalTest.h"

#include "react/engine/PulseCountEngine.h"
#include "react/engine/ToposortEngine.h"
#include "react/engine/SubtreeEngine.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;

INSTANTIATE_TYPED_TEST_CASE_P(SeqToposortQ, SignalTest, ToposortEngine<sequential_queue>);
INSTANTIATE_TYPED_TEST_CASE_P(ParToposortQ, SignalTest, ToposortEngine<parallel_queue>);
INSTANTIATE_TYPED_TEST_CASE_P(PulseCountQ, SignalTest, PulseCountEngine<parallel_queue>);
INSTANTIATE_TYPED_TEST_CASE_P(ParToposortP, SignalTest, ToposortEngine<parallel_pipeline>);
INSTANTIATE_TYPED_TEST_CASE_P(SubtreeQ, SignalTest, SubtreeEngine<parallel_queue>);

} // ~namespace