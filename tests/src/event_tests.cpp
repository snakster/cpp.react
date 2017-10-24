
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "gtest/gtest.h"

#include "react/event.h"
#include "react/observer.h"

#include <thread>
#include <chrono>

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