#include "ConversionTest.h"

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

INSTANTIATE_TYPED_TEST_CASE_P(Flooding, ConversionTest, FloodingEngine);
INSTANTIATE_TYPED_TEST_CASE_P(TopoSort, ConversionTest, TopoSortEngine);
INSTANTIATE_TYPED_TEST_CASE_P(PulseCount, ConversionTest, PulseCountEngine);
INSTANTIATE_TYPED_TEST_CASE_P(SourceSet, ConversionTest, SourceSetEngine);
INSTANTIATE_TYPED_TEST_CASE_P(TopoSortST, ConversionTest, TopoSortSTEngine);
INSTANTIATE_TYPED_TEST_CASE_P(PulseCountO1, ConversionTest, PulseCountO1Engine);
INSTANTIATE_TYPED_TEST_CASE_P(ELM, ConversionTest, ELMEngine);

} // ~namespace