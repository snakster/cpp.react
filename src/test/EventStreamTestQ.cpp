
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "EventStreamTest.h"

#include "react/engine/PulseCountEngine.h"
#include "react/engine/TopoSortEngine.h"
#include "react/engine/SourceSetEngine.h"
#include "react/engine/ELMEngine.h"
#include "react/engine/SubtreeEngine.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;

INSTANTIATE_TYPED_TEST_CASE_P(SeqTopoSortQ, EventStreamTest, TopoSortEngine<sequential_queue>);
INSTANTIATE_TYPED_TEST_CASE_P(ParTopoSortQ, EventStreamTest, TopoSortEngine<parallel_queue>);
INSTANTIATE_TYPED_TEST_CASE_P(ELMQ, EventStreamTest, ELMEngine<parallel_queue>);
INSTANTIATE_TYPED_TEST_CASE_P(PulseCountQ, EventStreamTest, PulseCountEngine<parallel_queue>);
INSTANTIATE_TYPED_TEST_CASE_P(SourceSetQ, EventStreamTest, SourceSetEngine<parallel_queue>);
INSTANTIATE_TYPED_TEST_CASE_P(ParTopoSortP, EventStreamTest, TopoSortEngine<parallel_pipeline>);
INSTANTIATE_TYPED_TEST_CASE_P(SubtreeQ, EventStreamTest, SubtreeEngine<parallel_queue>);

} // ~namespace