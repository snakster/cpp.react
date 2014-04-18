
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "gtest/gtest.h"

#include <queue>
#include <string>

#include "react/Algorithm.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;
using namespace std;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventStreamTest fixture
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TEngine>
class OperationsTest : public testing::Test
{
public:
    REACTIVE_DOMAIN(MyDomain, TEngine);
};

TYPED_TEST_CASE_P(OperationsTest);

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Fold1 test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(OperationsTest, Fold1)
{
    auto numSrc = MyDomain::MakeEventSource<int>();
    auto numFold = Fold(0, numSrc, [] (int v, int d) {
        return v + d;
    });

    for (auto i=1; i<=100; i++)
    {
        numSrc << i;
    }

    ASSERT_EQ(numFold(), 5050);

    auto charSrc = MyDomain::MakeEventSource<char>();
    auto strFold = Fold(string(""), charSrc, [] (string s, char c) {
        return s + c;
    });

    charSrc << 'T' << 'e' << 's' << 't';

    ASSERT_EQ(strFold(), "Test");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Fold2 test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(OperationsTest, Fold2)
{
    auto src = MyDomain::MakeEventSource<int>();
    auto f = Fold(0, src, [] (int v, int d) {
        return v + d;
    });

    int c = 0;

    Observe(f, [&] (int v) {
        c++;
        ASSERT_EQ(v, 5050);
    });

    MyDomain::DoTransaction([&] {
        for (auto i=1; i<=100; i++)
            src << i;
    });

    ASSERT_EQ(f(), 5050);
    ASSERT_EQ(c, 1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Iterate1 test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(OperationsTest, Iterate1)
{
    auto trigger = MyDomain::MakeEventSource();

    {
        auto inc = Iterate(0, trigger, Incrementer<int>{});
        for (auto i=1; i<=100; i++)
            trigger.Emit();

        ASSERT_EQ(inc(), 100);
    }

    {
        auto dec = Iterate(100, trigger, Decrementer<int>{});
        for (auto i=1; i<=100; i++)
            trigger.Emit();

        ASSERT_EQ(dec(), 0);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Monitor1
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(OperationsTest, Monitor1)
{
    auto target = MyDomain::MakeVar(10);

    vector<int> results;

    auto filterFunc = [] (int v) {
        return v > 10;
    };

    auto obs = Monitor(target).Filter(filterFunc).Observe([&] (int v) {
        results.push_back(v);
    });

    target <<= 10;
    target <<= 20;
    target <<= 20;
    target <<= 10;

    ASSERT_EQ(results.size(), 1);
    ASSERT_EQ(results[0], 20);

    obs.Detach();

    target <<= 100;

    ASSERT_EQ(results.size(), 1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Hold1
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(OperationsTest, Hold1)
{
    auto src = MyDomain::MakeEventSource<int>();

    auto h = Hold(0, src);

    ASSERT_EQ(h(), 0);

    src << 10;

    ASSERT_EQ(h(), 10);
    
    src << 20 << 30;

    ASSERT_EQ(h(), 30);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Hold1
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(OperationsTest, Pulse1)
{
    auto trigger = MyDomain::MakeEventSource();
    auto target = MyDomain::MakeVar(10);

    vector<int> results;

    auto p = Pulse(target, trigger);

    Observe(p, [&] (int v) {
        results.push_back(v);
    });

    target <<= 10;
    trigger.Emit();

    ASSERT_EQ(results[0], 10);

    target <<= 20;
    trigger.Emit();

    ASSERT_EQ(results[1], 20);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Snapshot1
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(OperationsTest, Snapshot1)
{
    auto trigger = MyDomain::MakeEventSource();
    auto target = MyDomain::MakeVar(10);

    auto snap = Snapshot(target, trigger);

    target <<= 10;
    trigger.Emit();
    target <<= 20;

    ASSERT_EQ(snap(), 10);

    target <<= 20;
    trigger.Emit();
    target <<= 30;

    ASSERT_EQ(snap(), 20);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
REGISTER_TYPED_TEST_CASE_P
(
    OperationsTest,
    Fold1,
    Fold2,
    Iterate1,
    Monitor1,
    Hold1,
    Pulse1,
    Snapshot1
);

} // ~namespace
