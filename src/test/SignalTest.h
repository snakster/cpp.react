
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "gtest/gtest.h"

#include <queue>

#include "react/Signal.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SignalTest fixture
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TEngine>
class SignalTest : public testing::Test
{
public:
    REACTIVE_DOMAIN(MyDomain, TEngine);
};

TYPED_TEST_CASE_P(SignalTest);

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MakeVars test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(SignalTest, MakeVars)
{
    auto v1 = MyDomain::MakeVar(1);
    auto v2 = MyDomain::MakeVar(2);
    auto v3 = MyDomain::MakeVar(3);
    auto v4 = MyDomain::MakeVar(4);

    ASSERT_EQ(v1(),1);
    ASSERT_EQ(v2(),2);
    ASSERT_EQ(v3(),3);
    ASSERT_EQ(v4(),4);

    v1 <<= 10;
    v2 <<= 20;
    v3 <<= 30;
    v4 <<= 40;

    ASSERT_EQ(v1(),10);
    ASSERT_EQ(v2(),20);
    ASSERT_EQ(v3(),30);
    ASSERT_EQ(v4(),40);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Signals1 test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(SignalTest, Signals1)
{
    auto v1 = MyDomain::MakeVar(1);
    auto v2 = MyDomain::MakeVar(2);
    auto v3 = MyDomain::MakeVar(3);
    auto v4 = MyDomain::MakeVar(4);

    auto s1 = MyDomain::MakeSignal
    (
        [] (int a, int b)
        {
            return a + b;
        },
        v1, v2
    );

    auto s2 = MyDomain::MakeSignal
    (
        [] (int a, int b)
        {
            return a + b;
        },
        v3, v4
    );

    auto s3 = s1 + s2;

    ASSERT_EQ(s1(),3);
    ASSERT_EQ(s2(),7);
    ASSERT_EQ(s3(),10);
    
    v1 <<= 10;
    v2 <<= 20;
    v3 <<= 30;
    v4 <<= 40;

    ASSERT_EQ(s1(),30);
    ASSERT_EQ(s2(),70);
    ASSERT_EQ(s3(),100);
    
    bool b = false;

    b = IsSignal<MyDomain, decltype(v1)>::value;
    ASSERT_TRUE(b);

    b = IsSignal<MyDomain, decltype(s1)>::value;
    ASSERT_TRUE(b);

    b = IsSignal<MyDomain, decltype(s2)>::value;
    ASSERT_TRUE(b);

    b = IsSignal<MyDomain, decltype(10)>::value;
    ASSERT_FALSE(b);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Signals2 test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(SignalTest, Signals2)
{
    auto a1 = MyDomain::MakeVar(1);
    auto a2 = MyDomain::MakeVar(1);
    
    auto b1 = a1 + 0;
    auto b2 = a1 + a2;
    auto b3 = a2 + 0;
    
    auto c1 = b1 + b2;
    auto c2 = b2 + b3;

    auto result = c1 + c2;

    int observeCount = 0;

    Observe(result, [&observeCount] (int v)
    {
        observeCount++;
        if (observeCount == 1)
            ASSERT_EQ(v,9);
        else
            ASSERT_EQ(v,12);
    });

    ASSERT_EQ(a1(),1);
    ASSERT_EQ(a2(),1);

    ASSERT_EQ(b1(),1);
    ASSERT_EQ(b2(),2);
    ASSERT_EQ(b3(),1);

    ASSERT_EQ(c1(),3);
    ASSERT_EQ(c2(),3);

    ASSERT_EQ(result(),6);

    a1 <<= 2;

    ASSERT_EQ(observeCount,1);

    ASSERT_EQ(a1(),2);
    ASSERT_EQ(a2(),1);

    ASSERT_EQ(b1(),2);
    ASSERT_EQ(b2(),3);
    ASSERT_EQ(b3(),1);

    ASSERT_EQ(c1(),5);
    ASSERT_EQ(c2(),4);

    ASSERT_EQ(result(),9);

    a2 <<= 2;

    ASSERT_EQ(observeCount,2);

    ASSERT_EQ(a1(),2);
    ASSERT_EQ(a2(),2);

    ASSERT_EQ(b1(),2);
    ASSERT_EQ(b2(),4);
    ASSERT_EQ(b3(),2);

    ASSERT_EQ(c1(),6);
    ASSERT_EQ(c2(),6);

    ASSERT_EQ(result(),12);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Signals3 test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(SignalTest, Signals3)
{
    auto a1 = MyDomain::MakeVar(1);
    auto a2 = MyDomain::MakeVar(1);
    
    auto b1 = a1 + 0;
    auto b2 = a1 + a2;
    auto b3 = a2 + 0;
    
    auto c1 = b1 + b2;
    auto c2 = b2 + b3;

    auto result = c1 + c2;

    int observeCount = 0;

    Observe(result, [&observeCount] (int v)
    {
        observeCount++;
        ASSERT_EQ(v,12);
    });

    ASSERT_EQ(a1(),1);
    ASSERT_EQ(a2(),1);

    ASSERT_EQ(b1(),1);
    ASSERT_EQ(b2(),2);
    ASSERT_EQ(b3(),1);

    ASSERT_EQ(c1(),3);
    ASSERT_EQ(c2(),3);

    ASSERT_EQ(result(),6);

    MyDomain::DoTransaction([&] {
        a1 <<= 2;
        a2 <<= 2;
    });

    ASSERT_EQ(observeCount,1);

    ASSERT_EQ(a1(),2);
    ASSERT_EQ(a2(),2);

    ASSERT_EQ(b1(),2);
    ASSERT_EQ(b2(),4);
    ASSERT_EQ(b3(),2);

    ASSERT_EQ(c1(),6);
    ASSERT_EQ(c2(),6);

    ASSERT_EQ(result(),12);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Signals4 test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(SignalTest, Signals4)
{
    auto a1 = MyDomain::MakeVar(1);
    auto a2 = MyDomain::MakeVar(1);
    
    auto b1 = a1 + a2;
    auto b2 = b1 + a2;

    ASSERT_EQ(a1(),1);
    ASSERT_EQ(a2(),1);

    ASSERT_EQ(b1(),2);
    ASSERT_EQ(b2(),3);

    a1 <<= 10;

    ASSERT_EQ(a1(),10);
    ASSERT_EQ(a2(),1);

    ASSERT_EQ(b1(),11);
    ASSERT_EQ(b2(),12);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// FunctionBind1 test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(SignalTest, FunctionBind1)
{
    auto v1 = MyDomain::MakeVar(2);
    auto v2 = MyDomain::MakeVar(30);
    auto v3 = MyDomain::MakeVar(10);

    auto signal = (v1, v2, v3) ->* [=] (int a, int b, int c) -> int
    {
        return a * b * c;
    };

    ASSERT_EQ(signal(),600);
    v3 <<= 100;
    ASSERT_EQ(signal(),6000);
}

int myfunc(int a, int b)        { return a + b; }
float myfunc2(int a)            { return a / 2.0f; }
float myfunc3(float a, float b) { return a * b; }

///////////////////////////////////////////////////////////////////////////////////////////////////
/// FunctionBind2 test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(SignalTest, FunctionBind2)
{
    auto a = MyDomain::MakeVar(1);
    auto b = MyDomain::MakeVar(1);
    
    auto c = ((a+b), (a+100)) ->* &myfunc;
    auto d = c ->* &myfunc2;
    auto e = (d,d) ->* &myfunc3;
    auto f = -e + 100;

    ASSERT_EQ(c(),103);
    ASSERT_EQ(d(),51.5f);
    ASSERT_EQ(e(),2652.25f);
    ASSERT_EQ(f(),-2552.25);

    a <<= 10;

    ASSERT_EQ(c(),121);
    ASSERT_EQ(d(),60.5f);
    ASSERT_EQ(e(),3660.25f);
    ASSERT_EQ(f(),-3560.25f);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Flatten1 test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(SignalTest, Flatten1)
{
    auto inner1 = MyDomain::MakeVar(123);
    auto inner2 = MyDomain::MakeVar(789);
    
    auto outer = MyDomain::MakeVar(inner1);

    auto flattened = Flatten(outer);

    std::queue<int> results;

    Observe(flattened, [&] (int v)
    {
        results.push(v);
    });

    ASSERT_TRUE(outer().Equals(inner1));
    ASSERT_EQ(flattened(),123);

    inner1 <<= 456;

    ASSERT_EQ(flattened(),456);

    ASSERT_EQ(results.front(), 456);
    results.pop();
    ASSERT_TRUE(results.empty());

    outer <<= inner2;

    ASSERT_TRUE(outer().Equals(inner2));
    ASSERT_EQ(flattened(),789);

    ASSERT_EQ(results.front(), 789);
    results.pop();
    ASSERT_TRUE(results.empty());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Flatten2 test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(SignalTest, Flatten2)
{    
    auto a0 = MyDomain::MakeVar(100);
    
    auto inner1 = MyDomain::MakeVar(200);

    auto a1 = MyDomain::MakeVar(300);
    auto a2 = a1 + 0;
    auto a3 = a2 + 0;
    auto a4 = a3 + 0;
    auto a5 = a4 + 0;
    auto a6 = a5 + 0;
    auto inner2 = a6 + 0;

    ASSERT_EQ(inner1(),200);
    ASSERT_EQ(inner2(),300);

    auto outer = MyDomain::MakeVar(inner1);

    auto flattened = Flatten(outer);

    ASSERT_EQ(flattened(), 200);

    int observeCount = 0;

    Observe(flattened, [&observeCount] (int v)
    {
        observeCount++;
    });

    auto o1 = a0 + flattened;
    auto o2 = o1 + 0;
    auto o3 = o2 + 0;
    auto result = o3 + 0;

    ASSERT_EQ(result(), 100 + 200);

    inner1 <<= 1234;

    ASSERT_EQ(result(), 100 + 1234);
    ASSERT_EQ(observeCount, 1);

    outer <<= inner2;

    ASSERT_EQ(result(), 100 + 300);
    ASSERT_EQ(observeCount, 2);

    MyDomain::DoTransaction([&] {
        a0 <<= 5000;
        a1 <<= 6000;
    });

    ASSERT_EQ(result(), 5000 + 6000);
    ASSERT_EQ(observeCount, 3);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Flatten3 test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(SignalTest, Flatten3)
{
    auto inner1 = MyDomain::MakeVar(10);

    auto a1 = MyDomain::MakeVar(20);
    auto a2 = a1 + 0;
    auto a3 = a2 + 0;
    auto inner2 = a3 + 0;

    auto outer = MyDomain::MakeVar(inner1);

    auto a0 = MyDomain::MakeVar(30);

    auto flattened = Flatten(outer);

    int observeCount = 0;

    Observe(flattened, [&observeCount] (int v)
    {
        observeCount++;
    });

    auto result = flattened + a0;

    ASSERT_EQ(result(), 10 + 30);
    ASSERT_EQ(observeCount, 0);

    MyDomain::DoTransaction([&] {
        inner1 <<= 1000;
        a0 <<= 200000;
        a1 <<= 50000;
        outer <<= inner2;
    });

    ASSERT_EQ(result(), 50000 + 200000);
    ASSERT_EQ(observeCount, 1);

    MyDomain::DoTransaction([&] {
        a0 <<= 667;
        a1 <<= 776;
    });

    ASSERT_EQ(result(), 776 + 667);
    ASSERT_EQ(observeCount, 2);

    MyDomain::DoTransaction([&] {
        inner1 <<= 999;
        a0 <<= 888;
    });

    ASSERT_EQ(result(), 776 + 888);
    ASSERT_EQ(observeCount, 2);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Flatten4 test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(SignalTest, Flatten4)
{
    std::vector<int> results;

    auto a1 = MyDomain::MakeVar(100);
    auto inner1 = a1 + 0;

    auto a2 = MyDomain::MakeVar(200);
    auto inner2 = a2;

    auto a3 = MyDomain::MakeVar(200);

    auto outer = MyDomain::MakeVar(inner1);

    auto flattened = Flatten(outer);

    auto result = flattened + a3;

    Observe(result, [&] (int v)    {
        results.push_back(v);
    });

    MyDomain::DoTransaction([&] {
        a3 <<= 400;
        outer <<= inner2;
    });

    ASSERT_EQ(results.size(), 1);

    ASSERT_TRUE(std::find(results.begin(), results.end(), 600) != results.end());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
REGISTER_TYPED_TEST_CASE_P
(
    SignalTest,
    MakeVars,
    Signals1, Signals2, Signals3, Signals4,
    FunctionBind1, FunctionBind2,
    Flatten1, Flatten2, Flatten3, Flatten4
);

} // ~namespace
