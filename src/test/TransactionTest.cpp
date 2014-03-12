
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "TransactionTest.h"

#include "react/propagation/FloodingEngine.h"
#include "react/propagation/PulseCountEngine.h"
#include "react/propagation/TopoSortEngine.h"
#include "react/propagation/SourceSetEngine.h"
#include "react/propagation/ELMEngine.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;

INSTANTIATE_TYPED_TEST_CASE_P(SeqTopoSort, TransactionTest, TopoSortEngine<sequential_queuing>);
INSTANTIATE_TYPED_TEST_CASE_P(ParTopoSort, TransactionTest, TopoSortEngine<parallel_queuing>);
INSTANTIATE_TYPED_TEST_CASE_P(Flooding, TransactionTest, FloodingEngine<parallel_queuing>);
INSTANTIATE_TYPED_TEST_CASE_P(ELM, TransactionTest, ELMEngine<parallel_queuing>);
INSTANTIATE_TYPED_TEST_CASE_P(PulseCount, TransactionTest, PulseCountEngine<parallel_queuing>);
INSTANTIATE_TYPED_TEST_CASE_P(SourceSet, TransactionTest, SourceSetEngine<parallel_queuing>);
INSTANTIATE_TYPED_TEST_CASE_P(ParTopoSortP, TransactionTest, TopoSortEngine<parallel_pipelining>);

// ---
}