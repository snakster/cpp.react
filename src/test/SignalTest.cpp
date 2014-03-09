#include "SignalTest.h"

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

INSTANTIATE_TYPED_TEST_CASE_P(Flooding, SignalTest, FloodingEngine<>);
INSTANTIATE_TYPED_TEST_CASE_P(TopoSort, SignalTest, TopoSortEngine<>);
INSTANTIATE_TYPED_TEST_CASE_P(PulseCount, SignalTest, PulseCountEngine<>);
INSTANTIATE_TYPED_TEST_CASE_P(SourceSet, SignalTest, SourceSetEngine<>);
INSTANTIATE_TYPED_TEST_CASE_P(TopoSortST, SignalTest, TopoSortEngine<sequential>);
INSTANTIATE_TYPED_TEST_CASE_P(PulseCountO1, SignalTest, PulseCountO1Engine<>);
INSTANTIATE_TYPED_TEST_CASE_P(ELM, SignalTest, ELMEngine<>);

// ---
}