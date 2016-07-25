
//          Copyright Sebastian Jeckel 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "gtest/gtest.h"

#include "react/Domain.h"
#include "react/Signal.h"
#include "react/Event.h"
#include "react/Observer.h"
#include "react/Algorithm.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;
using namespace std;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ParallelizationTest fixture
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TParams>
class ParallelizationTest : public testing::Test
{
public:
    template <EPropagationMode mode>
    class MyEngine : public TParams::template EngineT<mode> {};

    REACTIVE_DOMAIN(MyDomain, TParams::mode, MyEngine)
};

TYPED_TEST_CASE_P(ParallelizationTest);

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Iterate1 test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(ParallelizationTest, WeightHint1)
{
    using D = typename WeightHint1::MyDomain;

    auto sig = MakeVar<D>(0);
    auto evn = MakeEventSource<D,int>();
    auto cont = MakeContinuation<D>(sig, [] (int v) {});
    auto obs = Observe(evn, [] (int v) {});

    sig.SetWeightHint(WeightHint::heavy);
    evn.SetWeightHint(WeightHint::automatic);
    cont.SetWeightHint(WeightHint::light);
    obs.SetWeightHint(WeightHint::automatic);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
REGISTER_TYPED_TEST_CASE_P
(
    ParallelizationTest,
    WeightHint1
);

} // ~namespace
