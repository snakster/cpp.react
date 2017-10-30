
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "gtest/gtest.h"

#include "react/state.h"
#include "react/event.h"
#include "react/observer.h"

#include <thread>
#include <chrono>

using namespace react;

TEST(TransactionTest, Merging)
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

    // This transaction blocks the queue for one second.
    g.EnqueueTransaction([&]
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        });

    SyncPoint sp;

    // Enqueue 3 more transaction while the queue is blocked.
    // They should be merged together as a result.
    g.EnqueueTransaction([&]
        {
            evt.Emit(1);
            evt.Emit(2);
        }, sp, TransactionFlags::allow_merging);

    g.EnqueueTransaction([&]
        {
            evt.Emit(3);
            evt.Emit(4);
        }, sp, TransactionFlags::allow_merging);

    g.EnqueueTransaction([&]
        {
            evt.Emit(5);
            evt.Emit(6);
        }, sp, TransactionFlags::allow_merging);

    bool done = sp.WaitFor(std::chrono::seconds(3));
    EXPECT_EQ(true, done);

    // They have been merged, there should only be a single turn.
    EXPECT_EQ(1, turns);

    // None of the emitted values have been lost.
    EXPECT_EQ(21, output);
}

TEST(TransactionTest, LinkedSync)
{
    // Three groups. Each has one event with an observer attached.
    // The last observer adds a little delay.

    Group g1;
    Group g2;
    Group g3;

    auto evt1 = EventSource<int>::Create(g1);

    int output1 = 0;
    int turns1 = 0;

    auto obs1 = Observer::Create([&] (const auto& events)
        {
            ++turns1;
            for (int e : events)
                output1 += e;
        }, evt1);

    Event<int> evt2 = Filter(g2, [] (const auto&) { return true; }, evt1);

    int output2 = 0;
    int turns2 = 0;

    auto obs2 = Observer::Create([&] (const auto& events)
        {
            ++turns2;
            for (int e : events)
                output2 += e;
        }, evt2);

    Event<int> evt3 = Filter(g3, [] (const auto&) { return true; }, evt2);

    int output3 = 0;
    int turns3 = 0;

    auto obs3 = Observer::Create([&] (const auto& events)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            ++turns3;
            for (int e : events)
                output3 += e;
        }, evt3);

    SyncPoint sp;

    // Enqueue a transaction that waits on linked nodes.
    g1.EnqueueTransaction([&]
        {
            evt1.Emit(1);
            evt1.Emit(2);
        }, sp, TransactionFlags::sync_linked);

    // We should wait for all three observers.
    bool done = sp.WaitFor(std::chrono::seconds(3));

    EXPECT_EQ(true, done);

    EXPECT_EQ(1, turns1);
    EXPECT_EQ(1, turns2);
    EXPECT_EQ(1, turns3);

    EXPECT_EQ(3, output1);
    EXPECT_EQ(3, output2);
    EXPECT_EQ(3, output3);
}

TEST(TransactionTest, LinkedSyncMerging)
{
    // Two groups. Each has one event with an observer attached.
    // The last observer adds a little delay.

    Group g1;
    Group g2;

    auto evt1 = EventSource<int>::Create(g1);

    int output1 = 0;
    int turns1 = 0;

    auto obs1 = Observer::Create([&] (const auto& events)
        {
            ++turns1;
            for (int e : events)
                output1 += e;
        }, evt1);

    Event<int> evt2 = Filter(g2, [] (const auto&) { return true; }, evt1);

    int output2 = 0;
    int turns2 = 0;

    auto obs2 = Observer::Create([&] (const auto& events)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            ++turns2;
            for (int e : events)
                output2 += e;
        }, evt2);

    SyncPoint sp1;
    SyncPoint sp2;

    // This transaction blocks the queue for one second.
    g1.EnqueueTransaction([&]
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        });

    // Two more transactions are enqueued using two different sync points.
    // The first one should only sync on obs1, not on linked nodes.
    g1.EnqueueTransaction([&]
        {
            evt1.Emit(1);
            evt1.Emit(2);
        }, sp1, TransactionFlags::allow_merging);

    // The second one should sync on obs2 as well. 
    g1.EnqueueTransaction([&]
        {
            evt1.Emit(3);
            evt1.Emit(4);
        }, sp2, TransactionFlags::allow_merging | TransactionFlags::sync_linked);

    // Should be done after obs1 is done with both transactions (because they have been merged).
    bool done = sp1.WaitFor(std::chrono::seconds(5));

    EXPECT_EQ(true, done);

    EXPECT_EQ(1, turns1);
    EXPECT_EQ(0, turns2);

    EXPECT_EQ(10, output1);
    EXPECT_EQ(0, output2);

    // Should be done after obs2 is done.
    done = sp2.WaitFor(std::chrono::seconds(5));

    EXPECT_EQ(true, done);

    EXPECT_EQ(1, turns1);
    EXPECT_EQ(1, turns2);

    EXPECT_EQ(10, output1);
    EXPECT_EQ(10, output2);
}