#include "EventStreamTest.h"

#include "react/propagation/FloodingEngine.h"
#include "react/propagation/PulseCountEngine.h"
#include "react/propagation/TopoSortEngine.h"
#include "react/propagation/SourceSetEngine.h"
#include "react/propagation/TopoSortSTEngine.h"
#include "react/propagation/PulseCountO1Engine.h"
#include "react/propagation/ELMEngine.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;

INSTANTIATE_TYPED_TEST_CASE_P(Flooding, EventStreamTest, FloodingEngine);
INSTANTIATE_TYPED_TEST_CASE_P(TopoSort, EventStreamTest, TopoSortEngine);
INSTANTIATE_TYPED_TEST_CASE_P(PulseCount, EventStreamTest, PulseCountEngine);
INSTANTIATE_TYPED_TEST_CASE_P(SourceSet, EventStreamTest, SourceSetEngine);
INSTANTIATE_TYPED_TEST_CASE_P(TopoSortST, EventStreamTest, TopoSortSTEngine);
INSTANTIATE_TYPED_TEST_CASE_P(PulseCountO1, EventStreamTest, PulseCountO1Engine);
INSTANTIATE_TYPED_TEST_CASE_P(ELM, EventStreamTest, ELMEngine<>);

// ---
}