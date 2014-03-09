#include "SignalTest.h"

#include "react/propagation/FloodingEngine.h"
#include "react/propagation/PulseCountEngine.h"
#include "react/propagation/TopoSortEngine.h"
#include "react/propagation/SourceSetEngine.h"
#include "react/propagation/PulseCountO1Engine.h"
#include "react/propagation/ELMEngine.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;

INSTANTIATE_TYPED_TEST_CASE_P(SeqTopoSort, SignalTest, TopoSortEngine<sequential>);
INSTANTIATE_TYPED_TEST_CASE_P(ParTopoSort, SignalTest, TopoSortEngine<parallel>);
INSTANTIATE_TYPED_TEST_CASE_P(Flooding, SignalTest, FloodingEngine<parallel>);
INSTANTIATE_TYPED_TEST_CASE_P(ELM, SignalTest, ELMEngine<parallel>);
INSTANTIATE_TYPED_TEST_CASE_P(PulseCount, SignalTest, PulseCountEngine<parallel>);
INSTANTIATE_TYPED_TEST_CASE_P(SourceSet, SignalTest, SourceSetEngine<parallel>);
INSTANTIATE_TYPED_TEST_CASE_P(PulseCountO1, SignalTest, PulseCountO1Engine<parallel>);

// ---
}