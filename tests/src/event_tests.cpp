
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "gtest/gtest.h"

#include "react/event.h"
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
        EventSource<int> t1( g );
        EventSource<int> t2( t1 );
        EventSource<int> t3( std::move(t1) );
        
        EventSource<int> ref1( t2 );
        Event<int>       ref2( t3 );

        EXPECT_TRUE(ref1 == ref2);
    }

    // Event slot
    {
        EventSlot<int> t1( g );
        EventSlot<int> t2( t1 );
        EventSlot<int> t3( std::move(t1) );
        
        EventSlot<int> ref1( t2 );
        Event<int>     ref2( t3 );

        EXPECT_TRUE(ref1 == ref2);
    }

    // Event link
    {
        EventSlot<int> s1( g );

        EventLink<int> t1( g, s1 );
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

    EventSource<int> evt( g );

    int output = 0;

    Observer obs([&] (const auto& events)
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

    EventSource<int> evt1( g );
    EventSource<int> evt2( g );

    EventSlot<int> slot( g );

    int output = 0;
    int turns = 0;

    Observer obs([&] (const auto& events)
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

    EventSource<int> evt( g );

    int output = 0;
    int turns = 0;

    Observer obs([&] (const auto& events)
        {
            ++turns;
            for (int e : events)
                output += e;
        }, evt);

    EXPECT_EQ(0, output);

    g.DoTransaction([&]
        {
            evt.Emit(1);
            evt.Emit(1);
            evt.Emit(1);
            evt.Emit(1);
        });

    EXPECT_EQ(4, output);
    EXPECT_EQ(1, turns);
}

TEST(EventTest, Links)
{
    Group g1;
    Group g2;
    Group g3;

    EventSource<int> evt1( g1 );
    EventSource<int> evt2( g2 );
    EventSource<int> evt3( g3 );

    EventSlot<int> slot( g1 );

    // Same group
    slot.Add(evt1);

    // Explicit link
    EventLink<int> lnk2( g1, evt2 );
    slot.Add(lnk2);

    // Implicit link
    slot.Add(evt3);

    int output = 0;
    int turns = 0;

    EXPECT_EQ(0, output);

    Observer obs([&] (const auto& events)
        {
            ++turns;
            for (int e : events)
                output += e;
        }, slot);

    evt1.Emit(1);
    evt2.Emit(1);
    evt3.Emit(1);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    EXPECT_EQ(3, output);
    EXPECT_EQ(3, turns);
}

TEST(EventTest, EventSources)
{
    Group g;

    EventSource<int> es1( g );
    EventSource<int> es2( g );

    std::queue<int> results1;
    std::queue<int> results2;

    Observer obs1([&] (const auto& events)
        {
            for (int e : events)
                results1.push(e);
        }, es1);

    Observer obs2([&] (const auto& events)
        {
            for (int e : events)
                results2.push(e);
        }, es2);

    es1.Emit(10);
    es1.Emit(20);
    es1.Emit(30);

    es2.Emit(40);
    es2.Emit(50);
    es2.Emit(60);

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

    EventSource<int> a1( g );
    EventSource<int> a2( g );
    EventSource<int> a3( g );

    Event<int> merged = Merge(g, a1, a2, a3);

    std::vector<int> results;

    Observer obs1([&] (const auto& events)
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

    EventSource<std::string> a1( g );
    EventSource<std::string> a2( g );
    EventSource<std::string> a3( g );

    Event<std::string> merged = Merge(a1, a2, a3);

    std::vector<std::string> results;

    Observer obs1([&] (const auto& events)
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

    EventSource<int> a1( g );
    EventSource<int> a2( g );

    Event<int> f1 = Filter([] (int v) { return true; }, a1);
    Event<int> f2 = Filter([] (int v) { return true; }, a2);

    Event<int> merged = Merge(f1, f2);

    std::queue<int> results;

    Observer obs1([&] (const auto& events)
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

    EventSource<std::string> in( g );

    std::queue<std::string> results;

    Event<std::string> filtered = Filter(g, [] (const std::string& s)
        {
            return s == "Hello World";
        }, in);

    Observer obs1([&] (const auto& events)
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

    EventSource<std::string> in1( g );
    EventSource<std::string> in2( g );

    std::vector<std::string> results;

    Event<std::string> merged = Merge(in1, in2);

    Event<std::string> transformed = Transform<std::string>([] (std::string s) -> std::string
        {
            std::transform(s.begin(), s.end(), s.begin(), ::toupper);
            return s;
        }, merged);

    Observer obs1([&] (const auto& events)
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

TEST(EventTest, Chain)
{
    Group g;

    std::vector<float> results;

    EventSource<int> in1( g );
    EventSource<int> in2( g );

    auto merged = Merge(in1, in2);
    int turns = 0;

    Event<float> processed([&] (const auto& events, auto out)
        {
            for (const auto& e : events)
            {
                *out = 0.1f * e;
                *out = 1.5f * e;
            }

            ++turns;
        }, merged);

    Observer obs1([&] (const auto& events)
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

    EventSource<int> in1( g );
    EventSource<int> in2( g );
    EventSource<int> in3( g );

    Event<std::tuple<int, int, int>> joined = Join(in1, in2, in3);

    std::vector<std::tuple<int, int, int>> results;

    Observer obs1([&] (const auto& events)
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