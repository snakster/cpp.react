#include "OperationsTest.h"

#include "react/propagation/FloodingEngine.h"
#include "react/propagation/PulseCountEngine.h"
#include "react/propagation/TopoSortEngine.h"
#include "react/propagation/SourceSetEngine.h"
//#include "react/propagation/PulseCountO1Engine.h"
#include "react/propagation/ELMEngine.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;

INSTANTIATE_TYPED_TEST_CASE_P(SeqTopoSort, OperationsTest, TopoSortEngine<sequential>);
INSTANTIATE_TYPED_TEST_CASE_P(ParTopoSort, OperationsTest, TopoSortEngine<parallel>);
INSTANTIATE_TYPED_TEST_CASE_P(Flooding, OperationsTest, FloodingEngine<parallel>);
INSTANTIATE_TYPED_TEST_CASE_P(ELM, OperationsTest, ELMEngine<parallel>);
INSTANTIATE_TYPED_TEST_CASE_P(PulseCount, OperationsTest, PulseCountEngine<parallel>);
INSTANTIATE_TYPED_TEST_CASE_P(SourceSet, OperationsTest, SourceSetEngine<parallel>);
//INSTANTIATE_TYPED_TEST_CASE_P(PulseCountO1, OperationsTest, PulseCountO1Engine<parallel>);

} // ~namespace