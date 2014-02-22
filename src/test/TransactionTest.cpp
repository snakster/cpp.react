#include "TransactionTest.h"

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

INSTANTIATE_TYPED_TEST_CASE_P(Flooding, TransactionTest, FloodingEngine);
INSTANTIATE_TYPED_TEST_CASE_P(TopoSort, TransactionTest, TopoSortEngine);
INSTANTIATE_TYPED_TEST_CASE_P(PulseCount, TransactionTest, PulseCountEngine);
INSTANTIATE_TYPED_TEST_CASE_P(SourceSet, TransactionTest, SourceSetEngine);
INSTANTIATE_TYPED_TEST_CASE_P(PulseCountO1, TransactionTest, PulseCountO1Engine);
INSTANTIATE_TYPED_TEST_CASE_P(ELM, TransactionTest, ELMEngine);
// Merging not supported by TopoSortSTEngine
//INSTANTIATE_TYPED_TEST_CASE_P(TopoSortST, TransactionTest, TopoSortSTEngine);

// ---
}