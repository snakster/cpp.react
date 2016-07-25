
//          Copyright Sebastian Jeckel 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "gtest/gtest.h"

#include <vector>

#include "react/Domain.h"
#include "react/Signal.h"
#include "react/Event.h"
#include "react/Observer.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ObserverTest fixture
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TParams>
class ObserverTest : public testing::Test
{
public:
    template <EPropagationMode mode>
    class MyEngine : public TParams::template EngineT<mode> {};

    REACTIVE_DOMAIN(MyDomain, TParams::mode, MyEngine)
};

TYPED_TEST_CASE_P(ObserverTest);

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Detach test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(ObserverTest, Detach)
{
    using D = typename Detach::MyDomain;

    auto a1 = MakeVar<D>(1);
    auto a2 = MakeVar<D>(1);

    auto result = a1 + a2;

    int observeCount1 = 0;
    int observeCount2 = 0;
    int observeCount3 = 0;

    int phase;

    auto obs1 = Observe(result, [&] (int v)
    {
        observeCount1++;

        if (phase == 0)
            ASSERT_EQ(v,3);
        else if (phase == 1)
            ASSERT_EQ(v,4);
        else
            ASSERT_TRUE(false);
    });

    auto obs2 = Observe(result, [&] (int v)
    {
        observeCount2++;

        if (phase == 0)
            ASSERT_EQ(v,3);
        else if (phase == 1)
            ASSERT_EQ(v,4);
        else
            ASSERT_TRUE(false);
    });

    auto obs3 = Observe(result, [&] (int v)
    {
        observeCount3++;

        if (phase == 0)
            ASSERT_EQ(v,3);
        else if (phase == 1)
            ASSERT_EQ(v,4);
        else
            ASSERT_TRUE(false);
    });

    phase = 0;
    a1 <<= 2;
    ASSERT_EQ(observeCount1,1);
    ASSERT_EQ(observeCount2,1);
    ASSERT_EQ(observeCount3,1);

    phase = 1;
    obs1.Detach();
    a1 <<= 3;
    ASSERT_EQ(observeCount1,1);
    ASSERT_EQ(observeCount2,2);
    ASSERT_EQ(observeCount3,2);

    phase = 2;
    obs2.Detach();
    obs3.Detach();
    a1 <<= 4;
    ASSERT_EQ(observeCount1,1);
    ASSERT_EQ(observeCount2,2);
    ASSERT_EQ(observeCount3,2);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ScopedObserver test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(ObserverTest, ScopedObserverTest)
{
    using D = typename ScopedObserverTest::MyDomain;

    std::vector<int> results;

    auto in = MakeVar<D>(1);

    {
        ScopedObserver<D> obs = Observe(in, [&] (int v) {
            results.push_back(v);
        });

        in <<=2;
    }

    in <<=3;

    ASSERT_EQ(results.size(),1);
    ASSERT_EQ(results[0], 2);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Synced Observe test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(ObserverTest, SyncedObserveTest)
{
    using D = typename SyncedObserveTest::MyDomain;

    auto in1 = MakeVar<D>(1);
    auto in2 = MakeVar<D>(1);

    auto sum  = in1 + in2;
    auto prod = in1 * in2;
    auto diff = in1 - in2;

    auto src1 = MakeEventSource<D>();
    auto src2 = MakeEventSource<D,int>();

    Observe(src1, With(sum,prod,diff), [] (Token, int sum, int prod, int diff) {
        ASSERT_EQ(sum, 33);
        ASSERT_EQ(prod, 242);
        ASSERT_EQ(diff, 11);
    });

    Observe(src2, With(sum,prod,diff), [] (int e, int sum, int prod, int diff) {
        ASSERT_EQ(e, 42);
        ASSERT_EQ(sum, 33);
        ASSERT_EQ(prod, 242);
        ASSERT_EQ(diff, 11);
    });

    in1 <<= 22;
    in2 <<= 11;

    src1.Emit();
    src2.Emit(42);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// DetachThisObserver1 test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(ObserverTest, DetachThisObserver1)
{
    using D = typename DetachThisObserver1::MyDomain;

    auto src = MakeEventSource<D>();

    int count = 0;

    Observe(src, [&] (Token) -> ObserverAction {
        ++count;
        return ObserverAction::stop_and_detach;
    });

    src.Emit();
    src.Emit();

    printf("Count %d\n", count);
    ASSERT_EQ(count, 1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// DetachThisObserver2 test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(ObserverTest, DetachThisObserver2)
{
    using D = typename DetachThisObserver2::MyDomain;

    auto in1 = MakeVar<D>(1);
    auto in2 = MakeVar<D>(1);

    auto sum  = in1 + in2;
    auto prod = in1 * in2;
    auto diff = in1 - in2;

    auto src = MakeEventSource<D>();

    int count = 0;

    Observe(src, With(sum,prod,diff), [&] (Token, int sum, int prod, int diff) -> ObserverAction {
        ++count;
        return ObserverAction::stop_and_detach;
    });

    in1 <<= 22;
    in2 <<= 11;

    src.Emit();
    src.Emit();

    ASSERT_EQ(count, 1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
REGISTER_TYPED_TEST_CASE_P
(
    ObserverTest,
    Detach,
    ScopedObserverTest,
    SyncedObserveTest,
    DetachThisObserver1,
    DetachThisObserver2
);

} // ~namespace
