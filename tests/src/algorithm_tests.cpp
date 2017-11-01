
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "gtest/gtest.h"

#include "react/algorithm.h"
#include "react/observer.h"

#include <algorithm>
#include <chrono>
#include <queue>
#include <string>
#include <thread>
#include <tuple>

using namespace react;

TEST(AlgorithmTest, Hold)
{
    // Hold last value of event source in state.

    Group g;

    auto evt1 = EventSource<int>::Create(g);

    State<int> st = Hold(1, evt1);

    int output = 0;
    int turns = 0;

    auto obs = Observer::Create([&] (int v)
        {
            ++turns;
            output = v;
        }, st);

    // Initial call. Output should take the value of initial value.
    EXPECT_EQ(1, output);
    EXPECT_EQ(1, turns);

    // Event changes value.
    evt1.Emit(10);
    
    EXPECT_EQ(10, output);
    EXPECT_EQ(2, turns);

    // New event, but same value, observer should not be called.
    evt1.Emit(10);

    EXPECT_EQ(10, output);
    EXPECT_EQ(2, turns);
}

TEST(AlgorithmTest, Monitor1)
{
    // Emit events when value of state changes.

    Group g;

    auto st = StateVar<int>::Create(g, 1);

    Event<int> evt = Monitor( st );

    int output = 0;
    int turns = 0;

    auto obs = Observer::Create([&] (const auto& events)
        {
            ++turns;

            for (int e : events)
                output += e;
        }, evt);

    EXPECT_EQ(0, output);
    EXPECT_EQ(0, turns);

    // Change from 1 -> 10: Event.
    st.Set(10);

    // Change from 10 -> 20: Event.
    st.Set(20);

    // Change from 20 -> 20: No event.
    st.Set(20);

    // 10 + 20 were the changes.
    EXPECT_EQ(30, output);
    EXPECT_EQ(2, turns);
}

TEST(AlgorithmTest, Monitor2)
{
    // Monitor state changes and filter the resulting events.

    Group g;

    auto target = StateVar<int>::Create(g, 10);

    std::vector<int> results;

    auto filterFunc = [] (int v) { return v > 10; };

    {
        // Observer is created in a nested scope so it gets destructed before the end of this function.

        auto obs = Observer::Create([&] (const auto& events)
            {
                for (int e : events)
                    results.push_back(e);
            }, Filter(filterFunc, Monitor(target)));

        // Change the value a couple of times.
        target.Set(10); // Change, but <= 10
        target.Set(20); // Change
        target.Set(20); // No change
        target.Set(10); // Change, but <= 10

        // Only 1 non-filtered change should go through.
        EXPECT_EQ(results.size(), 1);
        EXPECT_EQ(results[0], 20);
    }

    target.Set(100); // Change, >100, but observer is gone.

    // No changes to results without the observer.
    ASSERT_EQ(results.size(), 1);
}

TEST(AlgorithmTest, Snapshot)
{
    Group g;

    auto sv = StateVar<int>::Create(g, 1);
    auto es = EventSource<>::Create(g);

    State<int> st = Snapshot( sv, es );

    int output = 0;
    int turns = 0;

    auto obs = Observer::Create([&] (int v)
        {
            ++turns;
            output = v;
        }, st);

    EXPECT_EQ(1, output);
    EXPECT_EQ(1, turns);

    sv.Set(10);

    EXPECT_EQ(1, output);
    EXPECT_EQ(1, turns);

    es.Emit();

    EXPECT_EQ(10, output);
    EXPECT_EQ(2, turns);
}

TEST(AlgorithmTest, Pulse)
{
    Group g;

    auto sv = StateVar<int>::Create(g, 1);
    auto es = EventSource<>::Create(g);

    Event<int> st = Pulse( sv, es );

    int output = 0;
    int turns = 0;

    auto obs = Observer::Create([&] (const auto& events)
        {
            for (int e : events)
            {
                ++turns;
                output += e;
            }
        }, st);

    EXPECT_EQ(0, output);
    EXPECT_EQ(0, turns);

    sv.Set(10);

    EXPECT_EQ(0, output);
    EXPECT_EQ(0, turns);

    es.Emit();

    EXPECT_EQ(10, output);
    EXPECT_EQ(1, turns);
}

TEST(AlgorithmTest, Iterate1)
{
    Group g;

    auto numSrc = EventSource<int>::Create(g);

    State<int> numFold = Iterate<int>(0, [] (const auto& events, int v)
        {
            for (int e : events)
                v += e;
            return v;
        }, numSrc);

    for (int i=1; i<=100; ++i)
        numSrc << i;

    int output = 0;
    auto obs = Observer::Create([&] (int v) { output = v; }, numFold);

    EXPECT_EQ(output, 5050);
}

TEST(AlgorithmTest, Iterate2)
{
    Group g;

    auto charSrc = EventSource<char>::Create(g);

    State<std::string> strFold = Iterate<std::string>(std::string(""), [] (const auto& events, std::string s)
        {
            for (char c : events)
                s += c;
            return s;
        }, charSrc);

    std::string output;
    auto obs = Observer::Create([&] (const auto& v) { output = v; }, strFold);

    charSrc << 'T' << 'e' << 's' << 't';

    EXPECT_EQ(output, "Test");
}

TEST(AlgorithmTest, Iterate3)
{
    Group g;

    auto numSrc = EventSource<int>::Create(g);

    State<int> numFold = Iterate<int>(0, [] (const auto& events, int v)
        {
            for (int e : events)
                v += e;
            return v;
        }, numSrc);

    int turns = 0;
    int output = 0;

    auto obs = Observer::Create([&] (const auto& v)
        {
            ++turns;
            output = v;
        }, numFold);

    g.DoTransaction([&]
        {
            for (int i=1; i<=100; i++)
                numSrc << i;
        });

    EXPECT_EQ(turns, 2);
    EXPECT_EQ(output, 5050);
}

template <typename T>
struct Incrementer
{
    T operator()(const EventValueList<Token>& events, T v) const
    { 
        for (auto e : events)
            ++v;
        return v;
    }
};

template <typename T>
struct Decrementer
{
    T operator()(const EventValueList<Token>& events, T v) const
    { 
        for (auto e : events)
            --v;
        return v;
    }
};

TEST(AlgorithmTest, Iterate4)
{
    Group g;

    auto trigger = EventSource<>::Create(g);

    {
        State<int> inc = Iterate<int>(0, Incrementer<int>{ }, trigger);
        for (int i=1; i<=100; i++)
            trigger.Emit();

        int output = 0;
        auto obs = Observer::Create([&] (int v) { output = v; }, inc);

        EXPECT_EQ(output, 100);
    }

    {
        State<int> dec = Iterate<int>(200, Decrementer<int>{ }, trigger);
        for (int i=1; i<=100; i++)
            trigger.Emit();

        int output = 0;
        auto obs = Observer::Create([&] (int v) { output = v; }, dec);

        ASSERT_EQ(output, 100);
    }
}

TEST(AlgorithmTest, IterateByRef1)
{
    Group g;

    auto src = EventSource<int>::Create(g);

    auto x = IterateByRef<std::vector<int>>(std::vector<int>{ }, [] (const auto& events, auto& v)
        {
            for (int e : events)
                v.push_back(e);
        }, src);

    std::vector<int> output;
    auto obs = Observer::Create([&] (const auto& v) { output = v; }, x);

    // Push
    for (int i=1; i<=100; i++)
        src << i;

    EXPECT_EQ(output.size(), 100);

    // Check
    for (int i=1; i<=100; i++)
        EXPECT_EQ(output[i-1], i);
}

TEST(AlgorithmTest, IterateByRef2)
{
    Group g;

    auto src = EventSource<>::Create(g);

    auto x = IterateByRef<std::vector<int>>(std::vector<int>{ }, [] (const auto& events, std::vector<int>& v)
        {
            for (Token e : events)
                v.push_back(123);
        }, src);

    std::vector<int> output;
    auto obs = Observer::Create([&] (const auto& v) { output = v; }, x);

    // Push
    for (auto i=0; i<100; i++)
        src.Emit();

    EXPECT_EQ(output.size(), 100);

    // Check
    for (auto i=0; i<100; i++)
        EXPECT_EQ(output[i], 123);
}

namespace {

template <typename T>
T Sum(T a, T b) { return a + b; }

template <typename T>
T Prod(T a, T b) { return a * b; }

template <typename T>
T Diff(T a, T b) { return a - b; }

} //~namespace

TEST(AlgorithmTest, TransformWithState)
{
    Group g;

    auto in1 = StateVar<int>::Create(g);
    auto in2 = StateVar<int>::Create(g);

    auto sum  = State<int>::Create(Sum<int>, in1, in2);
    auto prod = State<int>::Create(Prod<int>, in1, in2);
    auto diff = State<int>::Create(Diff<int>, in1, in2);

    auto src1 = EventSource<>::Create(g);
    auto src2 = EventSource<int>::Create(g);

    auto out1 = Transform<std::tuple<int, int, int>>([] (Token, int sum, int prod, int diff)
        {
            return std::make_tuple(sum, prod, diff);
        }, src1, sum, prod, diff);

    auto out2 = Transform<std::tuple<int, int, int, int>>([] (int e, int sum, int prod, int diff)
        {
            return std::make_tuple(e, sum, prod, diff);
        }, src2, sum, prod, diff);

    int turns1 = 0;
    int turns2 = 0;

    {
        std::tuple<int, int, int> output1;

        auto obs1 = Observer::Create([&] (const auto& events)
            {
                for (const auto& e : events)
                {
                    ++turns1;
                    output1 = e;
                }
            }, out1);

        std::tuple<int, int, int, int> output2;

        auto obs2 = Observer::Create([&] (const auto& events)
            {
                for (const auto& e : events)
                {
                    ++turns2;
                    output2 = e;
                }
            }, out2);

        in1.Set(22);
        in2.Set(11);

        src1.Emit();
        src2.Emit(42);

        EXPECT_EQ(std::get<0>(output1), 33);
        EXPECT_EQ(std::get<1>(output1), 242);
        EXPECT_EQ(std::get<2>(output1), 11);

        EXPECT_EQ(std::get<0>(output2), 42);
        EXPECT_EQ(std::get<1>(output2), 33);
        EXPECT_EQ(std::get<2>(output2), 242);
        EXPECT_EQ(std::get<3>(output2), 11);

        EXPECT_EQ(turns1, 1);
        EXPECT_EQ(turns2, 1);
    }

    {
        std::tuple<int, int, int> output1;

        auto obs1 = Observer::Create([&] (const auto& events)
            {
                for (const auto& e : events)
                {
                    ++turns1;
                    output1 = e;
                }
            }, out1);

        std::tuple<int, int, int, int> output2;

        auto obs2 = Observer::Create([&] (const auto& events)
            {
                for (const auto& e : events)
                {
                    ++turns2;
                    output2 = e;
                }
            }, out2);

        in1.Set(220);
        in2.Set(110);

        src1.Emit();
        src2.Emit(420);

        EXPECT_EQ(std::get<0>(output1), 330);
        EXPECT_EQ(std::get<1>(output1), 24200);
        EXPECT_EQ(std::get<2>(output1), 110);

        EXPECT_EQ(std::get<0>(output2), 420);
        EXPECT_EQ(std::get<1>(output2), 330);
        EXPECT_EQ(std::get<2>(output2), 24200);
        EXPECT_EQ(std::get<3>(output2), 110);

        EXPECT_EQ(turns1, 2);
        EXPECT_EQ(turns2, 2);
    }
}

TEST(AlgorithmTest, IterateWithState)
{
    Group g;

    auto in1 = StateVar<int>::Create(g);
    auto in2 = StateVar<int>::Create(g);

    auto op1 = State<int>::Create(Sum<int>, in1, in2);
    auto op2 = State<int>::Create([] (int a, int b) { return (a + b) * 10; }, in1, in2);

    auto src1 = EventSource<>::Create(g);
    auto src2 = EventSource<int>::Create(g);

    auto out1 = Iterate<std::tuple<int, int>>(std::make_tuple(0, 0), [] (const auto& events, std::tuple<int, int> t, int op1, int op2)
        {
            for (const auto& e : events)
                t = std::make_tuple(std::get<0>(t) + op1, std::get<1>(t) + op2);

            return t;
            
        }, src1, op1, op2);

    auto out2 = Iterate<std::tuple<int, int, int>>(std::make_tuple(0, 0, 0), [] (const auto& events, std::tuple<int, int, int> t, int op1, int op2)
        {
            for (const auto& e : events)
                t = std::make_tuple(std::get<0>(t) + e, std::get<1>(t) + op1, std::get<2>(t) + op2);

            return t;
        }, src2, op1, op2);

    int turns1 = 0;
    int turns2 = 0;

    {
        std::tuple<int, int> output1;

        auto obs1 = Observer::Create([&] (const auto& v)
            {
                ++turns1;
                output1 = v;
            }, out1);

        std::tuple<int, int, int> output2;

        auto obs2 = Observer::Create([&] (const auto& v)
            {
                ++turns2;
                output2 = v;
            }, out2);

        in1.Set(22);
        in2.Set(11);

        src1.Emit();
        src2.Emit(42);

        EXPECT_EQ(std::get<0>(output1), 33);
        EXPECT_EQ(std::get<1>(output1), 330);

        EXPECT_EQ(std::get<0>(output2), 42);
        EXPECT_EQ(std::get<1>(output2), 33);
        EXPECT_EQ(std::get<2>(output2), 330);

        EXPECT_EQ(turns1, 2);
        EXPECT_EQ(turns2, 2);
    }

    {
        std::tuple<int, int> output1;

        auto obs1 = Observer::Create([&] (const auto& v)
            {
                ++turns1;
                output1 = v;
            }, out1);

        std::tuple<int, int, int> output2;

        auto obs2 = Observer::Create([&] (const auto& v)
            {
                ++turns2;
                output2 = v;
            }, out2);

        in1.Set(220);
        in2.Set(110);

        src1.Emit();
        src2.Emit(420);

        EXPECT_EQ(std::get<0>(output1), 33 + 330);
        EXPECT_EQ(std::get<1>(output1), 330 + 3300);

        EXPECT_EQ(std::get<0>(output2), 42 + 420);
        EXPECT_EQ(std::get<1>(output2), 33 + 330);
        EXPECT_EQ(std::get<2>(output2), 330 + 3300);

        EXPECT_EQ(turns1, 4);
        EXPECT_EQ(turns2, 4);
    }
}

TEST(AlgorithmTest, IterateByRefWithState)
{
    Group g;

    auto in1 = StateVar<int>::Create(g);
    auto in2 = StateVar<int>::Create(g);

    auto op1 = State<int>::Create(Sum<int>, in1, in2);
    auto op2 = State<int>::Create([] (int a, int b) { return (a + b) * 10; }, in1, in2);

    auto src1 = EventSource<>::Create(g);
    auto src2 = EventSource<int>::Create(g);

    auto out1 = IterateByRef<std::vector<int>>(std::vector<int>{ }, [] (const auto& events, std::vector<int>& v, int op1, int op2)
        {
            for (const auto& e : events)
            {
                v.push_back(op1);
                v.push_back(op2);
            }
        }, src1, op1, op2);

    auto out2 = IterateByRef<std::vector<int>>(std::vector<int>{ }, [] (const auto& events, std::vector<int>& v, int op1, int op2)
        {
            for (const auto& e : events)
            {
                v.push_back(e);
                v.push_back(op1);
                v.push_back(op2);
            }
        }, src2, op1, op2);

    int turns1 = 0;
    int turns2 = 0;

    {
        std::vector<int> output1;

        auto obs1 = Observer::Create([&] (const std::vector<int>& v)
            {
                ++turns1;
                output1 = v;
            }, out1);

        std::vector<int> output2;

        auto obs2 = Observer::Create([&] (const std::vector<int>& v)
            {
                ++turns2;
                output2 = v;
            }, out2);

        in1.Set(22);
        in2.Set(11);

        src1.Emit();
        src2.Emit(42);

        EXPECT_EQ(output1.size(), 2);
        EXPECT_EQ(output1[0], 33);
        EXPECT_EQ(output1[1], 330);

        EXPECT_EQ(output2.size(), 3);
        EXPECT_EQ(output2[0], 42);
        EXPECT_EQ(output2[1], 33);
        EXPECT_EQ(output2[2], 330);

        EXPECT_EQ(turns1, 2);
        EXPECT_EQ(turns2, 2);
    }

    {
        std::vector<int> output1;

        auto obs1 = Observer::Create([&] (const std::vector<int>& v)
            {
                ++turns1;
                output1 = v;
            }, out1);

        std::vector<int> output2;

        auto obs2 = Observer::Create([&] (const std::vector<int>& v)
            {
                ++turns2;
                output2 = v;
            }, out2);

        in1.Set(220);
        in2.Set(110);

        src1.Emit();
        src2.Emit(420);

        EXPECT_EQ(output1.size(), 4);
        EXPECT_EQ(output1[0], 33);
        EXPECT_EQ(output1[1], 330);
        EXPECT_EQ(output1[2], 330);
        EXPECT_EQ(output1[3], 3300);

        EXPECT_EQ(output2.size(), 6);
        EXPECT_EQ(output2[0], 42);
        EXPECT_EQ(output2[1], 33);
        EXPECT_EQ(output2[2], 330);
        EXPECT_EQ(output2[3], 420);
        EXPECT_EQ(output2[4], 330);
        EXPECT_EQ(output2[5], 3300);

        EXPECT_EQ(turns1, 4);
        EXPECT_EQ(turns2, 4);
    }
}

Group flattenGroup;

class FlattenDummy
{
public:
    StateVar<int> value1 = StateVar<int>::Create(flattenGroup, 10);
    StateVar<int> value2 = StateVar<int>::Create(flattenGroup, 20);

    bool operator==(const FlattenDummy& other) const
        { return value1 == other.value1 && value2 == other.value2; }

    struct Flat;
};

struct FlattenDummy::Flat : public Flattened<FlattenDummy>
{
    using Flattened::Flattened;

    Ref<int> value1 = this->Flatten(FlattenDummy::value1);
    Ref<int> value2 = this->Flatten(FlattenDummy::value2);
};

TEST(AlgorithmTest, FlattenObject1)
{
    Group g;

    FlattenDummy o1;
    FlattenDummy o2;

    auto outer = StateVar<FlattenDummy>::Create(flattenGroup, o1);
    auto flat = FlattenObject(outer);

    int turns = 0;
    int output1 = 0;
    int output2 = 0;

    auto obs = Observer::Create([&] (const FlattenDummy::Flat& v)
        {
            ++turns;
            output1 = v.value1;
            output2 = v.value2;
        }, flat);

    EXPECT_EQ(turns, 1);
    EXPECT_EQ(output1, 10);
    EXPECT_EQ(output2, 20);

    o1.value1.Set(30);
    o1.value2.Set(40);

    EXPECT_EQ(turns, 3);
    EXPECT_EQ(output1, 30);
    EXPECT_EQ(output2, 40);

    outer.Set(o2);

    EXPECT_EQ(turns, 4);
    EXPECT_EQ(output1, 10);
    EXPECT_EQ(output2, 20);

    o1.value1.Set(300);
    o1.value2.Set(400);

    o2.value1.Set(500);
    o2.value2.Set(600);

    EXPECT_EQ(turns, 6);
    EXPECT_EQ(output1, 500);
    EXPECT_EQ(output2, 600);
}