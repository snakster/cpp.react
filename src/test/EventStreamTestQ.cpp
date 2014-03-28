
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "EventStreamTest.h"

#include "react/propagation/PulseCountEngine.h"
#include "react/propagation/TopoSortEngine.h"
#include "react/propagation/SourceSetEngine.h"
#include "react/propagation/ELMEngine.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;

INSTANTIATE_TYPED_TEST_CASE_P(SeqTopoSortQ, EventStreamTest, TopoSortEngine<sequential_queuing>);
INSTANTIATE_TYPED_TEST_CASE_P(ParTopoSortQ, EventStreamTest, TopoSortEngine<parallel_queuing>);
INSTANTIATE_TYPED_TEST_CASE_P(ELMQ, EventStreamTest, ELMEngine<parallel_queuing>);
INSTANTIATE_TYPED_TEST_CASE_P(PulseCountQ, EventStreamTest, PulseCountEngine<parallel_queuing>);
INSTANTIATE_TYPED_TEST_CASE_P(SourceSetQ, EventStreamTest, SourceSetEngine<parallel_queuing>);
INSTANTIATE_TYPED_TEST_CASE_P(ParTopoSortP, EventStreamTest, TopoSortEngine<parallel_pipelining>);

// ---
}