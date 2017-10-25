
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
        StateVar<int> t1( g, 0 );
        StateVar<int> t2( t1 );
        StateVar<int> t3( std::move(t1) );
        
        StateVar<int>   ref1( t2 );
        State<int>      ref2( t3 );

        EXPECT_TRUE(ref1 == ref2);
    }

    // State slot
    {
        StateVar<int> t0( g, 0 );

        StateSlot<int> t1( g, t0 );
        StateSlot<int> t2( t1 );
        StateSlot<int> t3( std::move(t1) );
        
        StateSlot<int> ref1( t2 );
        State<int>     ref2( t3 );

        EXPECT_TRUE(ref1 == ref2);
    }

    // State link
    {
        StateVar<int> t0( g, 0 );

        StateSlot<int> s1( g, t0 );

        StateLink<int> t1( g, s1 );
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

    StateVar<int> st( g );

    int output = 0;

    Observer obs([&] (const auto& v)
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

    StateVar<int> st1( g );
    StateVar<int> st2( g );

    StateSlot<int> slot( g, st1 );

    int output = 0;
    int turns = 0;

    Observer obs([&] (const auto& v)
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

    StateVar<int> st( g, 1 );

    int output = 0;
    int turns = 0;

    Observer obs([&] (const auto& v)
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

    StateVar<int> st1( g1, 1 );
    StateVar<int> st2( g2, 2 );
    StateVar<int> st3( g3, 3 );

    StateSlot<int> slot( g1, st1 );

    int output = 0;
    int turns = 0;

    Observer obs([&] (const auto& v)
        {
            ++turns;
            output = v;
        }, slot);

    EXPECT_EQ(1, turns);
    st1.Set(10);
    EXPECT_EQ(10, output);
    EXPECT_EQ(2, turns);

    // Explicit link
    StateLink<int> lnk2( g1, st2 );
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

    StateVar<int> a( g, 0 );
    StateVar<int> b( g, 0 );
    StateVar<int> c( g, 0 );

    State<int> s1(Sum2<int>, a, b);
    
    State<int> x(Sum2<int>, s1, c);
    State<int> y(Sum3<int>, a, b, c);

    int output1 = 0;
    int output2 = 0;
    int turns1 = 0;
    int turns2 = 0;

    Observer obs1([&] (int v)
        {
            ++turns1;
            output1 = v;
        }, x);

    EXPECT_EQ(0, output1);
    EXPECT_EQ(1, turns1);

    Observer obs2([&] (int v)
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

    StateVar<int> n1( g, 1 );

    State<int> n2([] (int n1)
        { return n1 + 1; }, n1);

    State<int> n3([] (int n1, int n2)
        { return n2 + n1 + 1; }, n1, n2);

    State<int> n4([] (int n3)
        { return n3 + 1; }, n3);

    State<int> n5([] (int n1, int n3, int n4)
        { return n4 + n3 + n1 + 1; }, n1, n3, n4);

    State<int> n6([] (int n5)
        { return n5 + 1; }, n5);

    State<int> n7([] (int n5, int n6)
        { return n6 + n5 + 1; }, n5, n6);

    State<int> n8([] (int n7)
        { return n7 + 1; }, n7);

    State<int> n9([] (int n1, int n5, int n7, int n8)
        { return n8 + n7 + n5 + n1 + 1; }, n1, n5, n7, n8);

    State<int> n10([] (int n9)
        { return n9 + 1; }, n9);

    State<int> n11([] (int n9, int n10)
        { return n10 + n9 + 1; }, n9, n10);

    State<int> n12([] (int n11)
        { return n11 + 1; }, n11);

    State<int> n13([] (int n9, int n11, int n12)
        { return n12 + n11 + n9 + 1; }, n9, n11, n12);

    State<int> n14([] (int n13)
        { return n13 + 1; }, n13);

    State<int> n15([] (int n13, int n14)
        { return n14 + n13 + 1; }, n13, n14);

    State<int> n16([] (int n15)
        { return n15 + 1; }, n15);

    State<int> n17([] (int n9, int n13, int n15, int n16)
        { return n16 + n15 + n13 + n9 + 1; }, n9, n13, n15, n16);

    Observer obs([&] (int v) { results.push_back(v); }, n17);

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

    StateVar<std::vector<int>> var( g, std::vector<int>{ } );

    int turns = 0;

    Observer obs([&] (const std::vector<int>& v)
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

    StateVar<std::vector<int>> var( g, std::vector<int>{ } );

    int turns = 0;

    Observer obs([&] (const std::vector<int>& v)
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

    StateVar<std::vector<int>> var( g, std::vector<int>{ } );

    int turns = 0;

    Observer obs([&] (const std::vector<int>& v)
        {
            ++turns;
            results = v;
        }, var);
    
    // Also terrible
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