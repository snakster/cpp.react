
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

INSTANTIATE_TYPED_TEST_CASE_P(SeqTopoSort, OperationsTest, TopoSortEngine<sequential>);
INSTANTIATE_TYPED_TEST_CASE_P(ParTopoSort, OperationsTest, TopoSortEngine<parallel>);
INSTANTIATE_TYPED_TEST_CASE_P(ELM, OperationsTest, ELMEngine<parallel>);
INSTANTIATE_TYPED_TEST_CASE_P(PulseCount, OperationsTest, PulseCountEngine<parallel>);
INSTANTIATE_TYPED_TEST_CASE_P(SourceSet, OperationsTest, SourceSetEngine<parallel>);
INSTANTIATE_TYPED_TEST_CASE_P(Subtree, OperationsTest, SubtreeEngine<parallel>);

} // ~namespace