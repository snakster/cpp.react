
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "gtest/gtest.h"

#include "react/event.h"
#include "react/state.h"
#include "react/observer.h"

#include <algorithm>
#include <chrono>
#include <queue>
#include <string>
#include <thread>
#include <tuple>

using namespace react;

TEST(EventTest, Construction)
{
    Group g;

    // Event source
    {
        auto t1 = EventSource<int>::Create(g);
        EventSource<int> t2( t1 );
        EventSource<int> t3( std::move(t1) );
        
        EventSource<int> ref1( t2 );
        Event<int>       ref2( t3 );

        EXPECT_TRUE(ref1 == ref2);
    }

    // Event slot
    {
        auto t1 = EventSlot<int>::Create(g);
        EventSlot<int> t2( t1 );
        EventSlot<int> t3( std::move(t1) );
        
        EventSlot<int> ref1( t2 );
        Event<int>     ref2( t3 );

        EXPECT_TRUE(ref1 == ref2);
    }

    // Event link
    {
        auto s1 = EventSlot<int>::Create(g);

        auto t1 = EventLink<int>::Create(g, s1);
        EventLink<int> t2( t1 );
        EventLink<int> t3( std::move(t1) );
        
        EventLink<int> ref1( t2 );
        Event<int>     ref2( t3 );

        EXPECT_TRUE(ref1 == ref2);
    }
}

TEST(EventTest, BasicOutput)
{
    Group g;

    auto evt = EventSource<int>::Create(g);

    int output = 0;

    auto obs = Observer::Create([&] (const auto& events)
        {
            for (int e : events)
                output += e;
        }, evt);

    EXPECT_EQ(0, output);

    evt.Emit(1);
    EXPECT_EQ(1, output);

    evt.Emit(2);
    EXPECT_EQ(3, output);
}

TEST(EventTest, Slots)
{
    Group g;

    auto evt1 = EventSource<int>::Create(g);
    auto evt2 = EventSource<int>::Create(g);

    auto slot = EventSlot<int>::Create(g);

    int output = 0;
    int turns = 0;

    auto obs = Observer::Create([&] (const auto& events)
        {
            ++turns;

            for (int e : events)
                output += e;
        }, slot);

    EXPECT_EQ(0, output);
    EXPECT_EQ(0, turns);

    slot.Add(evt1);
    slot.Add(evt2);

    evt1.Emit(5);
    evt2.Emit(2);

    EXPECT_EQ(7, output);
    EXPECT_EQ(2, turns);

    output = 0;

    slot.Remove(evt1);

    evt1.Emit(5);
    evt2.Emit(2);

    EXPECT_EQ(2, output);
    EXPECT_EQ(3, turns);

    output = 0;

    slot.Remove(evt2);

    evt1.Emit(5);
    evt2.Emit(2);

    EXPECT_EQ(0, output);
    EXPECT_EQ(3, turns);

    output = 0;

    slot.Add(evt1);
    slot.Add(evt1);

    evt1.Emit(5);
    evt2.Emit(2);

    EXPECT_EQ(5, output);
    EXPECT_EQ(4, turns);
}

TEST(EventTest, Transactions)
{
    Group g;

    auto evt = EventSource<int>::Create(g);

    int output = 0;
    int turns = 0;

    auto obs = Observer::Create([&] (const auto& events)
        {
            ++turns;
            for (int e : events)
                output += e;
        }, evt);

    EXPECT_EQ(0, output);

    g.DoTransaction([&]
        {
            evt << 1 << 1 << 1 << 1;
        });

    EXPECT_EQ(4, output);
    EXPECT_EQ(1, turns);
}

TEST(EventTest, Links)
{
    Group g1;
    Group g2;
    Group g3;

    auto evt1 = EventSource<int>::Create(g1);
    auto evt2 = EventSource<int>::Create(g2);
    auto evt3 = EventSource<int>::Create(g3);

    auto slot = EventSlot<int>::Create(g1);

    // Same group
    slot.Add(evt1);

    // Explicit link
    auto lnk2 = EventLink<int>::Create(g1, evt2);
    slot.Add(lnk2);

    // Implicit link
    slot.Add(evt3);

    int output = 0;
    int turns = 0;

    EXPECT_EQ(0, output);

    auto obs = Observer::Create([&] (const auto& events)
        {
            ++turns;
            for (int e : events)
                output += e;
        }, slot);

    evt1 << 1;
    evt2 << 1;
    evt3 << 1;

    std::this_thread::sleep_for(std::chrono::seconds(1));

    EXPECT_EQ(3, output);
    EXPECT_EQ(3, turns);
}

TEST(EventTest, EventSources)
{
    Group g;

    auto es1 = EventSource<int>::Create(g);
    auto es2 = EventSource<int>::Create(g);

    std::queue<int> results1;
    std::queue<int> results2;

    auto obs1 = Observer::Create([&] (const auto& events)
        {
            for (int e : events)
                results1.push(e);
        }, es1);

    auto obs2 = Observer::Create([&] (const auto& events)
        {
            for (int e : events)
                results2.push(e);
        }, es2);

    es1 << 10 << 20 << 30;
    es2 << 40 << 50 << 60;

    // First batch.
    EXPECT_FALSE(results1.empty());
    EXPECT_EQ(results1.front(), 10);
    results1.pop();

    EXPECT_FALSE(results1.empty());
    EXPECT_EQ(results1.front(), 20);
    results1.pop();

    EXPECT_FALSE(results1.empty());
    EXPECT_EQ(results1.front(),30);
    results1.pop();

    EXPECT_TRUE(results1.empty());

    // Second batch.
    EXPECT_FALSE(results2.empty());
    EXPECT_EQ(results2.front(),40);
    results2.pop();

    EXPECT_FALSE(results2.empty());
    EXPECT_EQ(results2.front(),50);
    results2.pop();

    EXPECT_FALSE(results2.empty());
    EXPECT_EQ(results2.front(),60);
    results2.pop();
    
    EXPECT_TRUE(results2.empty());
}

TEST(EventTest, Merge1)
{
    Group g;

    auto a1 = EventSource<int>::Create(g);
    auto a2 = EventSource<int>::Create(g);
    auto a3 = EventSource<int>::Create(g);

    Event<int> merged = Merge(g, a1, a2, a3);

    std::vector<int> results;

    auto obs1 = Observer::Create([&] (const auto& events)
        {
            for (int e : events)
                results.push_back(e);
        }, merged);

    g.DoTransaction([&]
        {
            a1.Emit(10);
            a2.Emit(20);
            a3.Emit(30);
        });

    EXPECT_EQ(results.size(), 3);

    EXPECT_TRUE(std::find(results.begin(), results.end(), 10) != results.end());
    EXPECT_TRUE(std::find(results.begin(), results.end(), 20) != results.end());
    EXPECT_TRUE(std::find(results.begin(), results.end(), 30) != results.end());
}

TEST(EventTest, Merge2)
{
    Group g;

    auto a1 = EventSource<std::string>::Create(g);
    auto a2 = EventSource<std::string>::Create(g);
    auto a3 = EventSource<std::string>::Create(g);

    Event<std::string> merged = Merge(a1, a2, a3);

    std::vector<std::string> results;

    auto obs1 = Observer::Create([&] (const auto& events)
        {
            for (const auto& e : events)
                results.push_back(e);
        }, merged);

    std::string s1("one");
    std::string s2("two");
    std::string s3("three");

    g.DoTransaction([&]
        {
            a1.Emit(s1);
            a2.Emit(s2);
            a3.Emit(s3);
        });

    EXPECT_EQ(results.size(), 3);

    EXPECT_TRUE(std::find(results.begin(), results.end(), "one") != results.end());
    EXPECT_TRUE(std::find(results.begin(), results.end(), "two") != results.end());
    EXPECT_TRUE(std::find(results.begin(), results.end(), "three") != results.end());
}

TEST(EventTest, Merge3)
{
    Group g;

    auto a1 = EventSource<int>::Create(g);
    auto a2 = EventSource<int>::Create(g);

    Event<int> f1 = Filter([] (int v) { return true; }, a1);
    Event<int> f2 = Filter([] (int v) { return true; }, a2);

    Event<int> merged = Merge(f1, f2);

    std::queue<int> results;

    auto obs1 = Observer::Create([&] (const auto& events)
        {
            for (int e : events)
                results.push(e);
        }, merged);

    a1.Emit(10);
    a2.Emit(20);
    a1.Emit(30);

    EXPECT_FALSE(results.empty());
    EXPECT_EQ(results.front(), 10);
    results.pop();

    EXPECT_FALSE(results.empty());
    EXPECT_EQ(results.front(), 20);
    results.pop();

    EXPECT_FALSE(results.empty());
    EXPECT_EQ(results.front(), 30);
    results.pop();

    EXPECT_TRUE(results.empty());
}

TEST(EventTest, Filter)
{
    Group g;

    auto in = EventSource<std::string>::Create(g);

    std::queue<std::string> results;

    Event<std::string> filtered = Filter(g, [] (const std::string& s)
        {
            return s == "Hello World";
        }, in);

    auto obs1 = Observer::Create([&] (const auto& events)
        {
            for (const auto& e : events)
                results.push(e);
        }, filtered);

    in.Emit(std::string("Hello Worlt"));
    in.Emit(std::string("Hello World"));
    in.Emit(std::string("Hello Vorld"));

    EXPECT_FALSE(results.empty());
    EXPECT_EQ(results.front(), "Hello World");
    results.pop();

    EXPECT_TRUE(results.empty());
}

TEST(EventTest, Transform)
{
    Group g;

    auto in1 = EventSource<std::string>::Create(g);
    auto in2 = EventSource<std::string>::Create(g);

    std::vector<std::string> results;

    Event<std::string> merged = Merge(in1, in2);

    Event<std::string> transformed = Transform<std::string>([] (std::string s) -> std::string
        {
            std::transform(s.begin(), s.end(), s.begin(), ::toupper);
            return s;
        }, merged);

    auto obs1 = Observer::Create([&] (const auto& events)
        {
            for (const auto& e : events)
                results.push_back(e);
        }, transformed);

    in1.Emit(std::string("Hello Worlt"));
    in1.Emit(std::string("Hello World"));
    in2.Emit(std::string("Hello Vorld"));

    EXPECT_EQ(results.size(), 3);
    EXPECT_TRUE(std::find(results.begin(), results.end(), "HELLO WORLT") != results.end());
    EXPECT_TRUE(std::find(results.begin(), results.end(), "HELLO WORLD") != results.end());
    EXPECT_TRUE(std::find(results.begin(), results.end(), "HELLO VORLD") != results.end());
}

TEST(EventTest, Flow)
{
    Group g;

    std::vector<float> results;

    auto in1 = EventSource<int>::Create(g);
    auto in2 = EventSource<int>::Create(g);

    auto merged = Merge(in1, in2);
    int turns = 0;

    auto processed = Event<float>::Create([&] (const auto& events, auto out)
        {
            for (const auto& e : events)
            {
                *out = 0.1f * e;
                *out = 1.5f * e;
            }

            ++turns;
        }, merged);

    auto obs1 = Observer::Create([&] (const auto& events)
        {
            for (float e : events)
                results.push_back(e);
        }, processed);

    g.DoTransaction([&] {
        in1.Emit(10);
        in1.Emit(20);
    });

    in2 << 30;

    EXPECT_EQ(results.size(), 6);
    EXPECT_EQ(turns, 2);

    EXPECT_EQ(results[0], 1.0f);
    EXPECT_EQ(results[1], 15.0f);
    EXPECT_EQ(results[2], 2.0f);
    EXPECT_EQ(results[3], 30.0f);
    EXPECT_EQ(results[4], 3.0f);
    EXPECT_EQ(results[5], 45.0f);
}

TEST(EventTest, Join)
{
    Group g;

    auto in1 = EventSource<int>::Create(g);
    auto in2 = EventSource<int>::Create(g);
    auto in3 = EventSource<int>::Create(g);

    Event<std::tuple<int, int, int>> joined = Join(in1, in2, in3);

    std::vector<std::tuple<int, int, int>> results;

    auto obs1 = Observer::Create([&] (const auto& events)
        {
            for (const auto& e : events)
                results.push_back(e);
        }, joined);

    in1.Emit(10);
    EXPECT_EQ(results.size(), 0);

    in2.Emit(10);
    EXPECT_EQ(results.size(), 0);

    in2.Emit(20);
    EXPECT_EQ(results.size(), 0);

    in3.Emit(10);
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0], std::make_tuple(10, 10, 10));

    in3.Emit(20);
    EXPECT_EQ(results.size(), 1);

    in1.Emit(20);
    EXPECT_EQ(results.size(), 2);
    EXPECT_EQ(results[1], std::make_tuple(20, 20, 20));
}

TEST(EventTest, FilterWithState)
{
    Group g;

    auto in = EventSource<std::string>::Create(g);

    auto sig1 = StateVar<int>::Create(g, 1338);
    auto sig2 = StateVar<int>::Create(g, 1336);

    auto in2 = EventSource<int>::Create(g);

    auto filtered = Filter([] (const std::string& s, int sig1, int sig2)
        {
            return s == "Hello World" && sig1 > sig2;
        }, in, sig1, sig2);

    std::queue<std::string> results;

    auto obs = Observer::Create([&] (const auto& events)
        {
            for (const auto& e : events)
                results.push(e);
        }, filtered);

    in << std::string("Hello Worlt") << std::string("Hello World") << std::string("Hello Vorld");
    sig1.Set(1335);
    in << std::string("Hello Vorld");

    EXPECT_FALSE(results.empty());
    EXPECT_EQ(results.front(), "Hello World");
    results.pop();

    EXPECT_TRUE(results.empty());
}

TEST(EventTest, TransformWithState)
{
    Group g;

    std::vector<std::string> results;

    auto in1 = EventSource<std::string>::Create(g);
    auto in2 = EventSource<std::string>::Create(g);

    Event<std::string> merged = Merge(in1, in2);

    auto first = StateVar<std::string>::Create(g, "Ace");
    auto last =  StateVar<std::string>::Create(g, "McSteele");

    auto transformed = Transform<std::string>([] (std::string s, const std::string& first, const std::string& last) -> std::string
        {
            std::transform(s.begin(), s.end(), s.begin(), ::toupper);
            s += std::string(", ") + first + std::string(" ") + last;
            return s;
        }, merged, first, last);

    auto obs = Observer::Create([&] (const auto& events)
        {
            for (const auto& e : events)
                results.push_back(e);
        }, transformed);

    in1 << std::string("Hello Worlt") << std::string("Hello World");

    g.DoTransaction([&]
        {
            in2 << std::string("Hello Vorld");
            first.Set(std::string("Alice"));
            last.Set(std::string("Anderson"));
        });

    EXPECT_EQ(results.size(), 3);
    EXPECT_TRUE(std::find(results.begin(), results.end(), "HELLO WORLT, Ace McSteele") != results.end());
    EXPECT_TRUE(std::find(results.begin(), results.end(), "HELLO WORLD, Ace McSteele") != results.end());
    EXPECT_TRUE(std::find(results.begin(), results.end(), "HELLO VORLD, Alice Anderson") != results.end());
}

TEST(EventTest, FlowWithState)
{
    Group g;

    std::vector<float> results;

    auto in1 = EventSource<int>::Create(g);
    auto in2 = EventSource<int>::Create(g);

    auto mult = StateVar<int>::Create(g, 10);

    Event<int> merged = Merge(in1, in2);
    int callCount = 0;

    auto processed = Event<float>::Create([&] (const auto& events, auto out, int mult)
        {
            for (const auto& e : events)
            {
                *out = 0.1f * e * mult;
                *out = 1.5f * e * mult;
            }

            callCount++;
        }, merged, mult);

    auto obs = Observer::Create([&] (const auto& events)
        {
            for (float e : events)
                results.push_back(e);
        }, processed);

    g.DoTransaction([&]
        {
            in1 << 10 << 20;
        });

    in2 << 30;

    EXPECT_EQ(results.size(), 6);
    EXPECT_EQ(callCount, 2);

    EXPECT_EQ(results[0], 10.0f);
    EXPECT_EQ(results[1], 150.0f);
    EXPECT_EQ(results[2], 20.0f);
    EXPECT_EQ(results[3], 300.0f);
    EXPECT_EQ(results[4], 30.0f);
    EXPECT_EQ(results[5], 450.0f);
}