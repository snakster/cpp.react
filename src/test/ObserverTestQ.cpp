
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "ObserverTest.h"

#include "react/propagation/FloodingEngine.h"
#include "react/propagation/PulseCountEngine.h"
#include "react/propagation/TopoSortEngine.h"
#include "react/propagation/SourceSetEngine.h"
#include "react/propagation/ELMEngine.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;

INSTANTIATE_TYPED_TEST_CASE_P(SeqTopoSortQ, ObserverTest, TopoSortEngine<sequential_queuing>);
INSTANTIATE_TYPED_TEST_CASE_P(ParTopoSortQ, ObserverTest, TopoSortEngine<parallel_queuing>);
INSTANTIATE_TYPED_TEST_CASE_P(FloodingQ, ObserverTest, FloodingEngine<parallel_queuing>);
INSTANTIATE_TYPED_TEST_CASE_P(ELMQ, ObserverTest, ELMEngine<parallel_queuing>);
INSTANTIATE_TYPED_TEST_CASE_P(PulseCountQ, ObserverTest, PulseCountEngine<parallel_queuing>);
INSTANTIATE_TYPED_TEST_CASE_P(SourceSetQ, ObserverTest, SourceSetEngine<parallel_queuing>);
INSTANTIATE_TYPED_TEST_CASE_P(ParTopoSortP, ObserverTest, TopoSortEngine<parallel_pipelining>);

// ---
}