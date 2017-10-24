
//          Copyright Sebastian Jeckel 2017.
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
template <typename TParams>
class OperationsTest : public testing::Test
{
public:
    template <EPropagationMode mode>
    class MyEngine : public TParams::template EngineT<mode> {};

    REACTIVE_DOMAIN(MyDomain, TParams::mode, MyEngine)
};

TYPED_TEST_CASE_P(OperationsTest);

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Iterate1 test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(OperationsTest, Iterate1)
{
    using D = typename Iterate1::MyDomain;

    auto numSrc = MakeEventSource<D,int>();
    auto numFold = Iterate(numSrc, 0, [] (int d, int v) {
        return v + d;
    });

    for (auto i=1; i<=100; i++)
    {
        numSrc << i;
    }

    ASSERT_EQ(numFold(), 5050);

    auto charSrc = MakeEventSource<D,char>();
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
    using D = typename Iterate2::MyDomain;

    auto numSrc = MakeEventSource<D,int>();
    auto numFold = Iterate(numSrc, 0, [] (int d, int v) {
        return v + d;
    });

    int c = 0;

    Observe(numFold, [&] (int v) {
        c++;
        ASSERT_EQ(v, 5050);
    });

    DoTransaction<D>([&] {
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
    using D = typename Iterate3::MyDomain;

    auto trigger = MakeEventSource<D>();

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
    using D = typename Monitor1::MyDomain;

    auto target = MakeVar<D>(10);

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
    using D = typename Hold1::MyDomain;

    auto src = MakeEventSource<D,int>();

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
    using D = typename Pulse1::MyDomain;

    auto trigger = MakeEventSource<D>();
    auto target = MakeVar<D>(10);

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
    using D = typename Snapshot1::MyDomain;

    auto trigger = MakeEventSource<D>();
    auto target = MakeVar<D>(10);

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
    using D = typename IterateByRef1::MyDomain;

    auto src = MakeEventSource<D,int>();
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
    using D = typename IterateByRef2::MyDomain;

    auto src = MakeEventSource<D>();
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
    using D = typename SyncedTransform1::MyDomain;

    auto in1 = MakeVar<D>(1);
    auto in2 = MakeVar<D>(1);

    auto sum  = in1 + in2;
    auto prod = in1 * in2;
    auto diff = in1 - in2;

    auto src1 = MakeEventSource<D>();
    auto src2 = MakeEventSource<D,int>();

    auto out1 = Transform(src1, With(sum,prod,diff), [] (Token, int sum, int prod, int diff) {
        return make_tuple(sum, prod, diff);
    });

    auto out2 = Transform(src2, With(sum,prod,diff), [] (int e, int sum, int prod, int diff) {
        return make_tuple(e, sum, prod, diff);
    });

    int obsCount1 = 0;
    int obsCount2 = 0;

    {
        auto obs1 = Observe(out1, [&] (const tuple<int,int,int>& t) {
            ++obsCount1;

            ASSERT_EQ(get<0>(t), 33);
            ASSERT_EQ(get<1>(t), 242);
            ASSERT_EQ(get<2>(t), 11);
        });

        auto obs2 =  Observe(out2, [&] (const tuple<int,int,int,int>& t) {
            ++obsCount2;

            ASSERT_EQ(get<0>(t), 42);
            ASSERT_EQ(get<1>(t), 33);
            ASSERT_EQ(get<2>(t), 242);
            ASSERT_EQ(get<3>(t), 11);
        });

        in1 <<= 22;
        in2 <<= 11;

        src1.Emit();
        src2.Emit(42);

        ASSERT_EQ(obsCount1, 1);
        ASSERT_EQ(obsCount2, 1);

        obs1.Detach();
        obs2.Detach();
    }

    {
        auto obs1 = Observe(out1, [&] (const tuple<int,int,int>& t) {
            ++obsCount1;

            ASSERT_EQ(get<0>(t), 330);
            ASSERT_EQ(get<1>(t), 24200);
            ASSERT_EQ(get<2>(t), 110);
        });

        auto obs2 = Observe(out2, [&] (const tuple<int,int,int,int>& t) {
            ++obsCount2;

            ASSERT_EQ(get<0>(t), 420);
            ASSERT_EQ(get<1>(t), 330);
            ASSERT_EQ(get<2>(t), 24200);
            ASSERT_EQ(get<3>(t), 110);
        });

        in1 <<= 220;
        in2 <<= 110;

        src1.Emit();
        src2.Emit(420);

        ASSERT_EQ(obsCount1, 2);
        ASSERT_EQ(obsCount2, 2);

        obs1.Detach();
        obs2.Detach();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncedIterate1 test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(OperationsTest, SyncedIterate1)
{
    using D = typename SyncedIterate1::MyDomain;

    auto in1 = MakeVar<D>(1);
    auto in2 = MakeVar<D>(1);

    auto op1 = in1 + in2;
    auto op2 = (in1 + in2) * 10;

    auto src1 = MakeEventSource<D>();
    auto src2 = MakeEventSource<D,int>();

    auto out1 = Iterate(
        src1,
        make_tuple(0,0),
        With(op1,op2),
        [] (Token, const tuple<int,int>& t, int op1, int op2) {
            return make_tuple(get<0>(t) + op1, get<1>(t) + op2);
        });

    auto out2 = Iterate(
        src2,
        make_tuple(0,0,0),
        With(op1,op2),
        [] (int e, const tuple<int,int,int>& t, int op1, int op2) {
            return make_tuple(get<0>(t) + e, get<1>(t) + op1, get<2>(t) + op2);
        });

    int obsCount1 = 0;
    int obsCount2 = 0;

    {
        auto obs1 = Observe(out1, [&] (const tuple<int,int>& t) {
            ++obsCount1;

            ASSERT_EQ(get<0>(t), 33);
            ASSERT_EQ(get<1>(t), 330);
        });

        auto obs2 = Observe(out2, [&] (const tuple<int,int,int>& t) {
            ++obsCount2;

            ASSERT_EQ(get<0>(t), 42);
            ASSERT_EQ(get<1>(t), 33);
            ASSERT_EQ(get<2>(t), 330);
        });

        in1 <<= 22;
        in2 <<= 11;

        src1.Emit();
        src2.Emit(42);

        ASSERT_EQ(obsCount1, 1);
        ASSERT_EQ(obsCount2, 1);

        obs1.Detach();
        obs2.Detach();
    }

    {
        auto obs1 = Observe(out1, [&] (const tuple<int,int>& t) {
            ++obsCount1;

            ASSERT_EQ(get<0>(t), 33 + 330);
            ASSERT_EQ(get<1>(t), 330 + 3300);
        });

        auto obs2 = Observe(out2, [&] (const tuple<int,int,int>& t) {
            ++obsCount2;

            ASSERT_EQ(get<0>(t), 42 + 420);
            ASSERT_EQ(get<1>(t), 33 + 330);
            ASSERT_EQ(get<2>(t), 330 + 3300);
        });

        in1 <<= 220;
        in2 <<= 110;

        src1.Emit();
        src2.Emit(420);

        ASSERT_EQ(obsCount1, 2);
        ASSERT_EQ(obsCount2, 2);

        obs1.Detach();
        obs2.Detach();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncedIterate2 test (by ref)
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(OperationsTest, SyncedIterate2)
{
    using D = typename SyncedIterate2::MyDomain;

    auto in1 = MakeVar<D>(1);
    auto in2 = MakeVar<D>(1);

    auto op1 = in1 + in2;
    auto op2 = (in1 + in2) * 10;

    auto src1 = MakeEventSource<D>();
    auto src2 = MakeEventSource<D,int>();

    auto out1 = Iterate(
        src1,
        vector<int>{},
        With(op1,op2),
        [] (Token, vector<int>& v, int op1, int op2) -> void {
            v.push_back(op1);
            v.push_back(op2);
        });

    auto out2 = Iterate(
        src2,
        vector<int>{},
        With(op1,op2),
        [] (int e, vector<int>& v, int op1, int op2) -> void {
            v.push_back(e);
            v.push_back(op1);
            v.push_back(op2);
        });

    int obsCount1 = 0;
    int obsCount2 = 0;

    {
        auto obs1 = Observe(out1, [&] (const vector<int>& v) {
            ++obsCount1;

            ASSERT_EQ(v.size(), 2);

            ASSERT_EQ(v[0], 33);
            ASSERT_EQ(v[1], 330);
        });

        auto obs2 = Observe(out2, [&] (const vector<int>& v) {
            ++obsCount2;

            ASSERT_EQ(v.size(), 3);

            ASSERT_EQ(v[0], 42);
            ASSERT_EQ(v[1], 33);
            ASSERT_EQ(v[2], 330);
        });

        in1 <<= 22;
        in2 <<= 11;

        src1.Emit();
        src2.Emit(42);

        ASSERT_EQ(obsCount1, 1);
        ASSERT_EQ(obsCount2, 1);

        obs1.Detach();
        obs2.Detach();
    }

    {
        auto obs1 = Observe(out1, [&] (const vector<int>& v) {
            ++obsCount1;

            ASSERT_EQ(v.size(), 4);

            ASSERT_EQ(v[0], 33);
            ASSERT_EQ(v[1], 330);
            ASSERT_EQ(v[2], 330);
            ASSERT_EQ(v[3], 3300);
        });

        auto obs2 = Observe(out2, [&] (const vector<int>& v) {
            ++obsCount2;

            ASSERT_EQ(v.size(), 6);

            ASSERT_EQ(v[0], 42);
            ASSERT_EQ(v[1], 33);
            ASSERT_EQ(v[2], 330);

            ASSERT_EQ(v[3], 420);
            ASSERT_EQ(v[4], 330);
            ASSERT_EQ(v[5], 3300);
        });

        in1 <<= 220;
        in2 <<= 110;

        src1.Emit();
        src2.Emit(420);

        ASSERT_EQ(obsCount1, 2);
        ASSERT_EQ(obsCount2, 2);

        obs1.Detach();
        obs2.Detach();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncedIterate3 test (event range)
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(OperationsTest, SyncedIterate3)
{
    using D = typename SyncedIterate3::MyDomain;

    auto in1 = MakeVar<D>(1);
    auto in2 = MakeVar<D>(1);

    auto op1 = in1 + in2;
    auto op2 = (in1 + in2) * 10;

    auto src1 = MakeEventSource<D>();
    auto src2 = MakeEventSource<D,int>();

    auto out1 = Iterate(
        src1,
        make_tuple(0,0),
        With(op1,op2),
        [] (EventRange<Token> range, const tuple<int,int>& t, int op1, int op2) {
            return make_tuple(
                get<0>(t) + (op1 * range.Size()),
                get<1>(t) + (op2 * range.Size()));
        });

    auto out2 = Iterate(
        src2,
        make_tuple(0,0,0),
        With(op1,op2),
        [] (EventRange<int> range, const tuple<int,int,int>& t, int op1, int op2) {
            int sum = 0;
            for (const auto& e : range)
                sum += e;

            return make_tuple(
                get<0>(t) + sum,
                get<1>(t) + (op1 * range.Size()),
                get<2>(t) + (op2 * range.Size()));
        });

    int obsCount1 = 0;
    int obsCount2 = 0;

    {
        auto obs1 = Observe(out1, [&] (const tuple<int,int>& t) {
            ++obsCount1;

            ASSERT_EQ(get<0>(t), 33);
            ASSERT_EQ(get<1>(t), 330);
        });

        auto obs2 = Observe(out2, [&] (const tuple<int,int,int>& t) {
            ++obsCount2;

            ASSERT_EQ(get<0>(t), 42);
            ASSERT_EQ(get<1>(t), 33);
            ASSERT_EQ(get<2>(t), 330);
        });

        in1 <<= 22;
        in2 <<= 11;

        src1.Emit();
        src2.Emit(42);

        ASSERT_EQ(obsCount1, 1);
        ASSERT_EQ(obsCount2, 1);

        obs1.Detach();
        obs2.Detach();
    }

    {
        auto obs1 = Observe(out1, [&] (const tuple<int,int>& t) {
            ++obsCount1;

            ASSERT_EQ(get<0>(t), 33 + 330);
            ASSERT_EQ(get<1>(t), 330 + 3300);
        });

        auto obs2 = Observe(out2, [&] (const tuple<int,int,int>& t) {
            ++obsCount2;

            ASSERT_EQ(get<0>(t), 42 + 420);
            ASSERT_EQ(get<1>(t), 33 + 330);
            ASSERT_EQ(get<2>(t), 330 + 3300);
        });

        in1 <<= 220;
        in2 <<= 110;

        src1.Emit();
        src2.Emit(420);

        ASSERT_EQ(obsCount1, 2);
        ASSERT_EQ(obsCount2, 2);

        obs1.Detach();
        obs2.Detach();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncedIterate4 test (event range, by ref)
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(OperationsTest, SyncedIterate4)
{
    using D = typename SyncedIterate4::MyDomain;

    auto in1 = MakeVar<D>(1);
    auto in2 = MakeVar<D>(1);

    auto op1 = in1 + in2;
    auto op2 = (in1 + in2) * 10;

    auto src1 = MakeEventSource<D>();
    auto src2 = MakeEventSource<D,int>();

    auto out1 = Iterate(
        src1,
        vector<int>{},
        With(op1,op2),
        [] (EventRange<Token> range, vector<int>& v, int op1, int op2) -> void {
            for (const auto& e : range)
            {
                v.push_back(op1);
                v.push_back(op2);
            }
        });

    auto out2 = Iterate(
        src2,
        vector<int>{},
        With(op1,op2),
        [] (EventRange<int> range, vector<int>& v, int op1, int op2) -> void {
            for (const auto& e : range)
            {
                v.push_back(e);
                v.push_back(op1);
                v.push_back(op2);
            }
        });

    int obsCount1 = 0;
    int obsCount2 = 0;

    {
        auto obs1 = Observe(out1, [&] (const vector<int>& v) {
            ++obsCount1;

            ASSERT_EQ(v.size(), 2);

            ASSERT_EQ(v[0], 33);
            ASSERT_EQ(v[1], 330);
        });

        auto obs2 = Observe(out2, [&] (const vector<int>& v) {
            ++obsCount2;

            ASSERT_EQ(v.size(), 3);

            ASSERT_EQ(v[0], 42);
            ASSERT_EQ(v[1], 33);
            ASSERT_EQ(v[2], 330);
        });

        in1 <<= 22;
        in2 <<= 11;

        src1.Emit();
        src2.Emit(42);

        ASSERT_EQ(obsCount1, 1);
        ASSERT_EQ(obsCount2, 1);

        obs1.Detach();
        obs2.Detach();
    }

    {
        auto obs1 = Observe(out1, [&] (const vector<int>& v) {
            ++obsCount1;

            ASSERT_EQ(v.size(), 4);

            ASSERT_EQ(v[0], 33);
            ASSERT_EQ(v[1], 330);
            ASSERT_EQ(v[2], 330);
            ASSERT_EQ(v[3], 3300);
        });

        auto obs2 = Observe(out2, [&] (const vector<int>& v) {
            ++obsCount2;

            ASSERT_EQ(v.size(), 6);

            ASSERT_EQ(v[0], 42);
            ASSERT_EQ(v[1], 33);
            ASSERT_EQ(v[2], 330);

            ASSERT_EQ(v[3], 420);
            ASSERT_EQ(v[4], 330);
            ASSERT_EQ(v[5], 3300);
        });

        in1 <<= 220;
        in2 <<= 110;

        src1.Emit();
        src2.Emit(420);

        ASSERT_EQ(obsCount1, 2);
        ASSERT_EQ(obsCount2, 2);

        obs1.Detach();
        obs2.Detach();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventFilter1
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(OperationsTest, SyncedEventFilter1)
{
    using D = typename SyncedEventFilter1::MyDomain;

    using std::string;

    std::queue<string> results;

    auto in = MakeEventSource<D,string>();

    auto sig1 = MakeVar<D>(1338);
    auto sig2 = MakeVar<D>(1336);

    auto filtered = Filter(
        in,
        With(sig1, sig2),
        [] (const string& s, int sig1, int sig2) {
            return s == "Hello World" && sig1 > sig2;
        });


    Observe(filtered, [&] (const string& s)
    {
        results.push(s);
    });

    in << string("Hello Worlt") << string("Hello World") << string("Hello Vorld");
    sig1 <<= 1335;
    in << string("Hello Vorld");

    ASSERT_FALSE(results.empty());
    ASSERT_EQ(results.front(), "Hello World");
    results.pop();

    ASSERT_TRUE(results.empty());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncedEventTransform1
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(OperationsTest, SyncedEventTransform1)
{
    using D = typename SyncedEventTransform1::MyDomain;

    using std::string;

    std::vector<string> results;

    auto in1 = MakeEventSource<D,string>();
    auto in2 = MakeEventSource<D,string>();

    auto merged = Merge(in1, in2);

    auto first = MakeVar<D>(string("Ace"));
    auto last = MakeVar<D>(string("McSteele"));

    auto transformed = Transform(
        merged,
        With(first, last),
        [] (string s, const string& first, const string& last) -> string {
            std::transform(s.begin(), s.end(), s.begin(), ::toupper);
            s += string(", ") + first + string(" ") + last;
            return s;
        });

    Observe(transformed, [&] (const string& s) {
        results.push_back(s);
    });

    in1 << string("Hello Worlt") << string("Hello World");

    DoTransaction<D>([&] {
        in2 << string("Hello Vorld");
        first.Set(string("Alice"));
        last.Set(string("Anderson"));
    });

    ASSERT_EQ(results.size(), 3);
    ASSERT_TRUE(std::find(results.begin(), results.end(), "HELLO WORLT, Ace McSteele") != results.end());
    ASSERT_TRUE(std::find(results.begin(), results.end(), "HELLO WORLD, Ace McSteele") != results.end());
    ASSERT_TRUE(std::find(results.begin(), results.end(), "HELLO VORLD, Alice Anderson") != results.end());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SyncedEventProcess1
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(OperationsTest, SyncedEventProcess1)
{
    using D = typename SyncedEventProcess1::MyDomain;

    std::vector<float> results;

    auto in1 = MakeEventSource<D,int>();
    auto in2 = MakeEventSource<D,int>();

    auto mult = MakeVar<D>(10);

    auto merged = Merge(in1, in2);
    int callCount = 0;

    auto processed = Process<float>(merged,
        With(mult),
        [&] (EventRange<int> range, EventEmitter<float> out, int mult)
        {
            for (const auto& e : range)
            {
                *out = 0.1f * e * mult;
                *out = 1.5f * e * mult;
            }

            callCount++;
        });

    Observe(processed,
        [&] (float s)
        {
            results.push_back(s);
        });

    DoTransaction<D>([&] {
        in1 << 10 << 20;
    });

    in2 << 30;

    ASSERT_EQ(results.size(), 6);
    ASSERT_EQ(callCount, 2);

    ASSERT_EQ(results[0], 10.0f);
    ASSERT_EQ(results[1], 150.0f);
    ASSERT_EQ(results[2], 20.0f);
    ASSERT_EQ(results[3], 300.0f);
    ASSERT_EQ(results[4], 30.0f);
    ASSERT_EQ(results[5], 450.0f);
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
    SyncedTransform1,
    SyncedIterate1,
    SyncedIterate2,
    SyncedIterate3,
    SyncedIterate4,
    SyncedEventFilter1,
    SyncedEventTransform1,
    SyncedEventProcess1
);

} // ~namespace
