#include "ObserverTest.h"

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

INSTANTIATE_TYPED_TEST_CASE_P(Flooding, ObserverTest, FloodingEngine<>);
INSTANTIATE_TYPED_TEST_CASE_P(TopoSort, ObserverTest, TopoSortEngine);
INSTANTIATE_TYPED_TEST_CASE_P(PulseCount, ObserverTest, PulseCountEngine<>);
INSTANTIATE_TYPED_TEST_CASE_P(SourceSet, ObserverTest, SourceSetEngine);
INSTANTIATE_TYPED_TEST_CASE_P(TopoSortST, ObserverTest, TopoSortSTEngine);
INSTANTIATE_TYPED_TEST_CASE_P(PulseCountO1, ObserverTest, PulseCountO1Engine);
INSTANTIATE_TYPED_TEST_CASE_P(ELM, ObserverTest, ELMEngine<>);

// ---
}