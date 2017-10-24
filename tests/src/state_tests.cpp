
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

TEST(StateTest, StateCombination)
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