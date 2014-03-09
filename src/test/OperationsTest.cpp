#include "OperationsTest.h"

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

INSTANTIATE_TYPED_TEST_CASE_P(Flooding, OperationsTest, FloodingEngine);
INSTANTIATE_TYPED_TEST_CASE_P(TopoSort, OperationsTest, TopoSortEngine);
INSTANTIATE_TYPED_TEST_CASE_P(PulseCount, OperationsTest, PulseCountEngine);
INSTANTIATE_TYPED_TEST_CASE_P(SourceSet, OperationsTest, SourceSetEngine);
INSTANTIATE_TYPED_TEST_CASE_P(TopoSortST, OperationsTest, TopoSortSTEngine);
INSTANTIATE_TYPED_TEST_CASE_P(PulseCountO1, OperationsTest, PulseCountO1Engine);
INSTANTIATE_TYPED_TEST_CASE_P(ELM, OperationsTest, ELMEngine<>);

} // ~namespace