
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "SignalTest.h"

#include "react/engine/PulseCountEngine.h"
#include "react/engine/TopoSortEngine.h"
#include "react/engine/SourceSetEngine.h"
#include "react/engine/ELMEngine.h"
#include "react/engine/SubtreeEngine.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;

INSTANTIATE_TYPED_TEST_CASE_P(SeqTopoSortQ, SignalTest, TopoSortEngine<sequential_queue>);
INSTANTIATE_TYPED_TEST_CASE_P(ParTopoSortQ, SignalTest, TopoSortEngine<parallel_queue>);
INSTANTIATE_TYPED_TEST_CASE_P(ELMQ, SignalTest, ELMEngine<parallel_queue>);
INSTANTIATE_TYPED_TEST_CASE_P(PulseCountQ, SignalTest, PulseCountEngine<parallel_queue>);
INSTANTIATE_TYPED_TEST_CASE_P(SourceSetQ, SignalTest, SourceSetEngine<parallel_queue>);
INSTANTIATE_TYPED_TEST_CASE_P(ParTopoSortP, SignalTest, TopoSortEngine<parallel_pipeline>);
INSTANTIATE_TYPED_TEST_CASE_P(SubtreeQ, SignalTest, SubtreeEngine<parallel_queue>);

} // ~namespace