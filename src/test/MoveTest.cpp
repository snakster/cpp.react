#include "MoveTest.h"

#include "react/propagation/TopoSortEngine.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;

INSTANTIATE_TYPED_TEST_CASE_P(SeqTopoSort, MoveTest, TopoSortEngine<sequential>);

// ---
}