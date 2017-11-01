
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "gtest/gtest.h"

#include "react/state.h"
#include "react/observer.h"

#include <thread>
#include <chrono>

using namespace react;

TEST(StateTest, Construction)
{
    Group g;

    // State variable
    {
        auto t1 = StateVar<int>::Create(g, 0);
        StateVar<int> t2( t1 );
        StateVar<int> t3( std::move(t1) );
        
        StateVar<int>   ref1( t2 );
        State<int>      ref2( t3 );

        EXPECT_TRUE(ref1 == ref2);
    }

    // State slot
    {
        auto t0 = StateVar<int>::Create(g, 0);

        auto t1 = StateSlot<int>::Create(g, t0);
        StateSlot<int> t2( t1 );
        StateSlot<int> t3( std::move(t1) );
        
        StateSlot<int> ref1( t2 );
        State<int>     ref2( t3 );

        EXPECT_TRUE(ref1 == ref2);
    }

    // State link
    {
        auto t0 = StateVar<int>::Create(g, 0);

        auto s1 = StateSlot<int>::Create(g, t0);

        auto t1 = StateLink<int>::Create(g, s1);
        StateLink<int> t2( t1 );
        StateLink<int> t3( std::move(t1) );
        
        StateLink<int> ref1( t2 );
        State<int>     ref2( t3 );

        EXPECT_TRUE(ref1 == ref2);
    }
}

TEST(StateTest, BasicOutput)
{
    Group g;

    auto st = StateVar<int>::Create(g);

    int output = 0;

    auto obs2 = Observer::Create([&] (const auto& v)
        {
            output += v;
        }, st);

    EXPECT_EQ(0, output);

    st.Set(1);
    EXPECT_EQ(1, output);

    st.Set(2);
    EXPECT_EQ(3, output);
}

TEST(StateTest, Slots)
{
    Group g;

    auto st1 = StateVar<int>::Create(g);
    auto st2 = StateVar<int>::Create(g);

    auto slot = StateSlot<int>::Create(g, st1);

    int output = 0;
    int turns = 0;

    auto obs = Observer::Create([&] (const auto& v)
        {
            ++turns;
            output += v;
        }, slot);

    EXPECT_EQ(0, output);
    EXPECT_EQ(1, turns);

    slot.Set(st1);
    st1.Set(5);
    st2.Set(2);

    EXPECT_EQ(5, output);
    EXPECT_EQ(2, turns);

    output = 0;

    slot.Set(st2);
    st1.Set(5);
    st2.Set(2);

    EXPECT_EQ(2, output);
    EXPECT_EQ(3, turns);
}

TEST(StateTest, Transactions)
{
    Group g;

    auto st = StateVar<int>::Create(g, 1);

    int output = 0;
    int turns = 0;

    auto obs = Observer::Create([&] (const auto& v)
        {
            ++turns;
            output += v;
        }, st);

    EXPECT_EQ(1, output);

    g.DoTransaction([&]
        {
            st.Set(1);
            st.Set(2);
            st.Set(3);
            st.Set(4);
        });

    EXPECT_EQ(5, output);
    EXPECT_EQ(2, turns);
}

TEST(StateTest, Links)
{
    Group g1;
    Group g2;
    Group g3;

    auto st1 = StateVar<int>::Create(g1, 1);
    auto st2 = StateVar<int>::Create(g2, 2);
    auto st3 = StateVar<int>::Create(g3, 3);

    auto slot = StateSlot<int>::Create(g1, st1);

    int output = 0;
    int turns = 0;

    auto obs = Observer::Create([&] (const auto& v)
        {
            ++turns;
            output = v;
        }, slot);

    EXPECT_EQ(1, turns);
    st1.Set(10);
    EXPECT_EQ(10, output);
    EXPECT_EQ(2, turns);

    // Explicit link
    auto lnk2 = StateLink<int>::Create(g1, st2);
    slot.Set(lnk2);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(2, output);
    EXPECT_EQ(3, turns);

    st2.Set(20);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(20, output);
    EXPECT_EQ(4, turns);

    // Implicit link
    slot.Set(st3);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(3, output);
    EXPECT_EQ(5, turns);

    st3.Set(30);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(30, output);
    EXPECT_EQ(6, turns);

    std::this_thread::sleep_for(std::chrono::seconds(1));
}

namespace
{

template <typename T>
static T Sum2(T a, T b)
{
    return a + b;
}

template <typename T>
static T Sum3(T a, T b, T c)
{
    return a + b + c;
}

} // ~namespace

TEST(StateTest, StateCombination1)
{
    Group g;

    auto a = StateVar<int>::Create(g, 0);
    auto b = StateVar<int>::Create(g, 0);
    auto c = StateVar<int>::Create(g, 0);

    auto s1 = State<int>::Create(Sum2<int>, a, b);
    
    auto x = State<int>::Create(Sum2<int>, s1, c);
    auto y = State<int>::Create(Sum3<int>, a, b, c);

    int output1 = 0;
    int output2 = 0;
    int turns1 = 0;
    int turns2 = 0;

    auto obs1 = Observer::Create([&] (int v)
        {
            ++turns1;
            output1 = v;
        }, x);

    EXPECT_EQ(0, output1);
    EXPECT_EQ(1, turns1);

    auto obs2 = Observer::Create([&] (int v)
        {
            ++turns2;
            output2 = v;
        }, y);

    EXPECT_EQ(0, output2);
    EXPECT_EQ(1, turns2);

    a.Set(1);
    b.Set(1);
    c.Set(1);

    EXPECT_EQ(3, output1);
    EXPECT_EQ(4, turns1);

    EXPECT_EQ(3, output2);
    EXPECT_EQ(4, turns2);
}

TEST(StateTest, StateCombination2)
{
    Group g;

    std::vector<int> results;

    auto n1 = StateVar<int>::Create(g, 1);

    auto n2 = State<int>::Create([] (int n1)
        { return n1 + 1; }, n1);

    auto n3 = State<int>::Create([] (int n1, int n2)
        { return n2 + n1 + 1; }, n1, n2);

    auto n4 = State<int>::Create([] (int n3)
        { return n3 + 1; }, n3);

    auto n5 = State<int>::Create([] (int n1, int n3, int n4)
        { return n4 + n3 + n1 + 1; }, n1, n3, n4);

    auto n6 = State<int>::Create([] (int n5)
        { return n5 + 1; }, n5);

    auto n7 = State<int>::Create([] (int n5, int n6)
        { return n6 + n5 + 1; }, n5, n6);

    auto n8 = State<int>::Create([] (int n7)
        { return n7 + 1; }, n7);

    auto n9 = State<int>::Create([] (int n1, int n5, int n7, int n8)
        { return n8 + n7 + n5 + n1 + 1; }, n1, n5, n7, n8);

    auto n10 = State<int>::Create([] (int n9)
        { return n9 + 1; }, n9);

    auto n11 = State<int>::Create([] (int n9, int n10)
        { return n10 + n9 + 1; }, n9, n10);

    auto n12 = State<int>::Create([] (int n11)
        { return n11 + 1; }, n11);

    auto n13 = State<int>::Create([] (int n9, int n11, int n12)
        { return n12 + n11 + n9 + 1; }, n9, n11, n12);

    auto n14 = State<int>::Create([] (int n13)
        { return n13 + 1; }, n13);

    auto n15 = State<int>::Create([] (int n13, int n14)
        { return n14 + n13 + 1; }, n13, n14);

    auto n16 = State<int>::Create([] (int n15)
        { return n15 + 1; }, n15);

    auto n17 = State<int>::Create([] (int n9, int n13, int n15, int n16)
        { return n16 + n15 + n13 + n9 + 1; }, n9, n13, n15, n16);

    auto obs = Observer::Create([&] (int v) { results.push_back(v); }, n17);

    n1.Set(10);     // 7732
    n1.Set(100);    // 68572
    n1.Set(1000);   // 676972

    EXPECT_EQ(results.size(), 4);

    EXPECT_EQ(results[0], 1648);
    EXPECT_EQ(results[1], 7732);
    EXPECT_EQ(results[2], 68572);
    EXPECT_EQ(results[3], 676972);
}

TEST(StateTest, Modify1)
{
    Group g;

    std::vector<int> results;

    auto var = StateVar<std::vector<int>>::Create(g, std::vector<int>{ });

    int turns = 0;

    auto obs = Observer::Create([&] (const std::vector<int>& v)
        {
            ++turns;
            results = v;
        }, var);
    
    var.Modify([] (std::vector<int>& v)
        {
            v.push_back(30);
            v.push_back(50);
            v.push_back(70);
        });

    EXPECT_EQ(results[0], 30);
    EXPECT_EQ(results[1], 50);
    EXPECT_EQ(results[2], 70);

    EXPECT_EQ(turns, 2);
}

TEST(StateTest, Modify2)
{
    Group g;

    std::vector<int> results;

    auto var = StateVar<std::vector<int>>::Create(g, std::vector<int>{ });

    int turns = 0;

    auto obs = Observer::Create([&] (const std::vector<int>& v)
        {
            ++turns;
            results = v;
        }, var);
    
    g.DoTransaction([&]
        {
            var.Modify([] (std::vector<int>& v) { v.push_back(30); });
            var.Modify([] (std::vector<int>& v) { v.push_back(50); });
            var.Modify([] (std::vector<int>& v) { v.push_back(70); });
        });

    EXPECT_EQ(results[0], 30);
    EXPECT_EQ(results[1], 50);
    EXPECT_EQ(results[2], 70);

    EXPECT_EQ(turns, 2);
}

TEST(StateTest, Modify3)
{
    Group g;

    std::vector<int> results;

    auto var = StateVar<std::vector<int>>::Create(g, std::vector<int>{ });

    int turns = 0;

    auto obs = Observer::Create([&] (const std::vector<int>& v)
        {
            ++turns;
            results = v;
        }, var);
    
    g.DoTransaction([&]
        {
            var.Set(std::vector<int>{ 30, 50 });
            var.Modify([] (std::vector<int>& v) { v.push_back(70); });
        });

    EXPECT_EQ(results[0], 30);
    EXPECT_EQ(results[1], 50);
    EXPECT_EQ(results[2], 70);

    ASSERT_EQ(turns, 2);
}
