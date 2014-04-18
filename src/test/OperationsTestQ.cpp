
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "OperationsTest.h"

#include "react/engine/PulseCountEngine.h"
#include "react/engine/TopoSortEngine.h"
#include "react/engine/SourceSetEngine.h"
#include "react/engine/ELMEngine.h"
#include "react/engine/SubtreeEngine.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;

INSTANTIATE_TYPED_TEST_CASE_P(SeqTopoSortQ, OperationsTest, TopoSortEngine<sequential_queue>);
INSTANTIATE_TYPED_TEST_CASE_P(ParTopoSortQ, OperationsTest, TopoSortEngine<parallel_queue>);
INSTANTIATE_TYPED_TEST_CASE_P(ELMQ, OperationsTest, ELMEngine<parallel_queue>);
INSTANTIATE_TYPED_TEST_CASE_P(PulseCountQ, OperationsTest, PulseCountEngine<parallel_queue>);
INSTANTIATE_TYPED_TEST_CASE_P(SourceSetQ, OperationsTest, SourceSetEngine<parallel_queue>);
INSTANTIATE_TYPED_TEST_CASE_P(ParTopoSortP, OperationsTest, TopoSortEngine<parallel_pipeline>);
INSTANTIATE_TYPED_TEST_CASE_P(SubtreeQ, OperationsTest, SubtreeEngine<parallel_queue>);

} // ~namespace