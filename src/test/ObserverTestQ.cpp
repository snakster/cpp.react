
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "ObserverTest.h"

#include "react/engine/PulseCountEngine.h"
#include "react/engine/TopoSortEngine.h"
#include "react/engine/SourceSetEngine.h"
#include "react/engine/ELMEngine.h"
#include "react/engine/SubtreeEngine.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;

INSTANTIATE_TYPED_TEST_CASE_P(SeqTopoSortQ, ObserverTest, TopoSortEngine<sequential_queue>);
INSTANTIATE_TYPED_TEST_CASE_P(ParTopoSortQ, ObserverTest, TopoSortEngine<parallel_queue>);
INSTANTIATE_TYPED_TEST_CASE_P(ELMQ, ObserverTest, ELMEngine<parallel_queue>);
INSTANTIATE_TYPED_TEST_CASE_P(PulseCountQ, ObserverTest, PulseCountEngine<parallel_queue>);
INSTANTIATE_TYPED_TEST_CASE_P(SourceSetQ, ObserverTest, SourceSetEngine<parallel_queue>);
INSTANTIATE_TYPED_TEST_CASE_P(ParTopoSortP, ObserverTest, TopoSortEngine<parallel_pipeline>);
INSTANTIATE_TYPED_TEST_CASE_P(SubtreeQ, ObserverTest, SubtreeEngine<parallel_queue>);

} // ~namespace