
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "SignalTest.h"

#include "react/propagation/FloodingEngine.h"
#include "react/propagation/PulseCountEngine.h"
#include "react/propagation/TopoSortEngine.h"
#include "react/propagation/SourceSetEngine.h"
#include "react/propagation/ELMEngine.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;

INSTANTIATE_TYPED_TEST_CASE_P(SeqTopoSortQ, SignalTest, TopoSortEngine<sequential_queuing>);
INSTANTIATE_TYPED_TEST_CASE_P(ParTopoSortQ, SignalTest, TopoSortEngine<parallel_queuing>);
INSTANTIATE_TYPED_TEST_CASE_P(FloodingQ, SignalTest, FloodingEngine<parallel_queuing>);
INSTANTIATE_TYPED_TEST_CASE_P(ELMQ, SignalTest, ELMEngine<parallel_queuing>);
INSTANTIATE_TYPED_TEST_CASE_P(PulseCountQ, SignalTest, PulseCountEngine<parallel_queuing>);
INSTANTIATE_TYPED_TEST_CASE_P(SourceSetQ, SignalTest, SourceSetEngine<parallel_queuing>);
INSTANTIATE_TYPED_TEST_CASE_P(ParTopoSortP, SignalTest, TopoSortEngine<parallel_pipelining>);

// ---
}