
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "ObserverTest.h"

#include "react/propagation/PulseCountEngine.h"
#include "react/propagation/TopoSortEngine.h"
#include "react/propagation/SourceSetEngine.h"
#include "react/propagation/ELMEngine.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;

INSTANTIATE_TYPED_TEST_CASE_P(SeqTopoSort, ObserverTest, TopoSortEngine<sequential>);
INSTANTIATE_TYPED_TEST_CASE_P(ParTopoSort, ObserverTest, TopoSortEngine<parallel>);
INSTANTIATE_TYPED_TEST_CASE_P(ELM, ObserverTest, ELMEngine<parallel>);
INSTANTIATE_TYPED_TEST_CASE_P(PulseCount, ObserverTest, PulseCountEngine<parallel>);
INSTANTIATE_TYPED_TEST_CASE_P(SourceSet, ObserverTest, SourceSetEngine<parallel>);

// ---
}