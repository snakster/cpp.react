
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "gtest/gtest.h"

#include <queue>
#include <string>
#include <tuple>

#include "react/Domain.h"
#include "react/Signal.h"
#include "react/Event.h"
#include "react/Observer.h"
#include "react/Algorithm.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;
using namespace std;

template <typename T>
struct Incrementer { T operator()(Token, T v) const { return v+1; } };

template <typename T>
struct Decrementer { T operator()(Token, T v) const { return v-1; } };

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
/// Iterate1 test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(OperationsTest, Iterate1)
{
    auto numSrc = MyDomain::MakeEventSource<int>();
    auto numFold = Iterate(numSrc, 0, [] (int d, int v) {
        return v + d;
    });

    for (auto i=1; i<=100; i++)
    {
        numSrc << i;
    }

    ASSERT_EQ(numFold(), 5050);

    auto charSrc = MyDomain::MakeEventSource<char>();
    auto strFold = Iterate(charSrc, string(""), [] (char c, string s) {
        return s + c;
    });

    charSrc << 'T' << 'e' << 's' << 't';

    ASSERT_EQ(strFold(), "Test");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Iterate2 test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(OperationsTest, Iterate2)
{
    auto numSrc = MyDomain::MakeEventSource<int>();
    auto numFold = Iterate(numSrc, 0, [] (int d, int v) {
        return v + d;
    });

    int c = 0;

    Observe(numFold, [&] (int v) {
        c++;
        ASSERT_EQ(v, 5050);
    });

    MyDomain::DoTransaction([&] {
        for (auto i=1; i<=100; i++)
            numSrc << i;
    });

    ASSERT_EQ(numFold(), 5050);
    ASSERT_EQ(c, 1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Iterate3 test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(OperationsTest, Iterate3)
{
    auto trigger = MyDomain::MakeEventSource();

    {
        auto inc = Iterate(trigger, 0, Incrementer<int>{});
        for (auto i=1; i<=100; i++)
            trigger.Emit();

        ASSERT_EQ(inc(), 100);
    }

    {
        auto dec = Iterate(trigger, 100, Decrementer<int>{});
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

    auto obs = Observe(Monitor(target).Filter(filterFunc), [&] (int v) {
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

    auto h = Hold(src, 0);

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

    auto p = Pulse(trigger, target);

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

    auto snap = Snapshot(trigger, target);

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
/// IterateByRef1 test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(OperationsTest, IterateByRef1)
{
    auto src = MyDomain::MakeEventSource<int>();
    auto f = Iterate(
        src,
        std::vector<int>(),
        [] (int d, std::vector<int>& v) {
            v.push_back(d);
        });

    // Push
    for (auto i=1; i<=100; i++)
        src << i;

    ASSERT_EQ(f().size(), 100);

    // Check
    for (auto i=1; i<=100; i++)
        ASSERT_EQ(f()[i-1], i);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IterateByRef2 test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(OperationsTest, IterateByRef2)
{
    auto src = MyDomain::MakeEventSource();
    auto x = Iterate(
        src,
        std::vector<int>(),
        [] (Token, std::vector<int>& v) {
            v.push_back(123);
        });

    // Push
    for (auto i=0; i<100; i++)
        src.Emit();

    ASSERT_EQ(x().size(), 100);

    // Check
    for (auto i=0; i<100; i++)
        ASSERT_EQ(x()[i], 123);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncedTransform1 test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(OperationsTest, SyncedTransform1)
{
    auto in1 = MyDomain::MakeVar(1);
    auto in2 = MyDomain::MakeVar(1);

    auto sum  = in1 + in2;
    auto prod = in1 * in2;
    auto diff = in1 - in2;

    auto src1 = MyDomain::MakeEventSource();
    auto src2 = MyDomain::MakeEventSource<int>();

    auto out1 = Transform(src1, With(sum,prod,diff), [] (Token, int sum, int prod, int diff) {
        return make_tuple(sum, prod, diff);
    });

    auto out2 = Transform(src2, With(sum,prod,diff), [] (int e, int sum, int prod, int diff) {
        return make_tuple(e, sum, prod, diff);
    });

    int obsCount1 = 0;
    int obsCount2 = 0;

    Observe(out1, [&] (const tuple<int,int,int>& t) {
        ++obsCount1;

        ASSERT_EQ(get<0>(t), 33);
        ASSERT_EQ(get<1>(t), 242);
        ASSERT_EQ(get<2>(t), 11);

        DetachThisObserver();
    });

    Observe(out2, [&] (const tuple<int,int,int,int>& t) {
        ++obsCount2;

        ASSERT_EQ(get<0>(t), 42);
        ASSERT_EQ(get<1>(t), 33);
        ASSERT_EQ(get<2>(t), 242);
        ASSERT_EQ(get<3>(t), 11);

        DetachThisObserver();
    });

    in1 <<= 22;
    in2 <<= 11;

    src1.Emit();
    src2.Emit(42);

    ASSERT_EQ(obsCount1, 1);
    ASSERT_EQ(obsCount2, 1);

    Observe(out1, [&] (const tuple<int,int,int>& t) {
        ++obsCount1;

        ASSERT_EQ(get<0>(t), 330);
        ASSERT_EQ(get<1>(t), 24200);
        ASSERT_EQ(get<2>(t), 110);

        DetachThisObserver();
    });

    Observe(out2, [&] (const tuple<int,int,int,int>& t) {
        ++obsCount2;

        ASSERT_EQ(get<0>(t), 420);
        ASSERT_EQ(get<1>(t), 330);
        ASSERT_EQ(get<2>(t), 24200);
        ASSERT_EQ(get<3>(t), 110);

        DetachThisObserver();
    });

    in1 <<= 220;
    in2 <<= 110;

    src1.Emit();
    src2.Emit(420);

    ASSERT_EQ(obsCount1, 2);
    ASSERT_EQ(obsCount2, 2);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
REGISTER_TYPED_TEST_CASE_P
(
    OperationsTest,
    Iterate1,
    Iterate2,
    Iterate3,
    Monitor1,
    Hold1,
    Pulse1,
    Snapshot1,
    IterateByRef1,
    IterateByRef2,
    SyncedTransform1
);

} // ~namespace
