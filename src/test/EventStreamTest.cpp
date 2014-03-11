#include "EventStreamTest.h"

#include "react/propagation/FloodingEngine.h"
#include "react/propagation/PulseCountEngine.h"
#include "react/propagation/TopoSortEngine.h"
#include "react/propagation/SourceSetEngine.h"
#include "react/propagation/ELMEngine.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;

INSTANTIATE_TYPED_TEST_CASE_P(SeqTopoSort, EventStreamTest, TopoSortEngine<sequential>);
INSTANTIATE_TYPED_TEST_CASE_P(ParTopoSort, EventStreamTest, TopoSortEngine<parallel>);
INSTANTIATE_TYPED_TEST_CASE_P(Flooding, EventStreamTest, FloodingEngine<parallel>);
INSTANTIATE_TYPED_TEST_CASE_P(ELM, EventStreamTest, ELMEngine<parallel>);
INSTANTIATE_TYPED_TEST_CASE_P(PulseCount, EventStreamTest, PulseCountEngine<parallel>);
INSTANTIATE_TYPED_TEST_CASE_P(SourceSet, EventStreamTest, SourceSetEngine<parallel>);

// ---
}