
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <queue>
#include <string>

#include "gtest/gtest.h"

#include "react/Domain.h"
#include "react/Event.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventStreamTest fixture
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TParams>
class EventStreamTest : public testing::Test
{
public:
    template <EPropagationMode mode>
    class MyEngine : public TParams::template EngineT<mode> {};

    REACTIVE_DOMAIN(MyDomain, TParams::mode, MyEngine)
};

TYPED_TEST_CASE_P(EventStreamTest);

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventSources test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(EventStreamTest, EventSources)
{
    using D = typename EventSources::MyDomain;

    auto es1 = MakeEventSource<D,int>();
    auto es2 = MakeEventSource<D,int>();

    std::queue<int> results1;
    std::queue<int> results2;

    Observe(es1, [&] (int v)
    {
        results1.push(v);
    });

    Observe(es2, [&] (int v)
    {
        results2.push(v);
    });

    es1 << 10 << 20 << 30;
    es2 << 40 << 50 << 60;

    // 1
    ASSERT_FALSE(results1.empty());
    ASSERT_EQ(results1.front(),10);
    results1.pop();

    ASSERT_FALSE(results1.empty());
    ASSERT_EQ(results1.front(),20);
    results1.pop();

    ASSERT_FALSE(results1.empty());
    ASSERT_EQ(results1.front(),30);
    results1.pop();

    ASSERT_TRUE(results1.empty());

    // 2
    ASSERT_FALSE(results2.empty());
    ASSERT_EQ(results2.front(),40);
    results2.pop();

    ASSERT_FALSE(results2.empty());
    ASSERT_EQ(results2.front(),50);
    results2.pop();

    ASSERT_FALSE(results2.empty());
    ASSERT_EQ(results2.front(),60);
    results2.pop();
    
    ASSERT_TRUE(results2.empty());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventMerge1 test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(EventStreamTest, EventMerge1)
{
    using D = typename EventMerge1::MyDomain;

    auto a1 = MakeEventSource<D,int>();
    auto a2 = MakeEventSource<D,int>();
    auto a3 = MakeEventSource<D,int>();

    auto merged = Merge(a1, a2, a3);

    std::vector<int> results;

    Observe(merged, [&] (int v)
    {
        results.push_back(v);
    });
    
    DoTransaction<D>([&] {
        a1 << 10;
        a2 << 20;
        a3 << 30;
    });

    ASSERT_EQ(results.size(), 3);
    ASSERT_TRUE(std::find(results.begin(), results.end(), 10) != results.end());
    ASSERT_TRUE(std::find(results.begin(), results.end(), 20) != results.end());
    ASSERT_TRUE(std::find(results.begin(), results.end(), 30) != results.end());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventMerge2 test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(EventStreamTest, EventMerge2)
{
    using D = typename EventMerge2::MyDomain;

    auto a1 = MakeEventSource<D,std::string>();
    auto a2 = MakeEventSource<D,std::string>();
    auto a3 = MakeEventSource<D,std::string>();

    auto merged = Merge(a1, a2, a3);

    std::vector<std::string> results;

    Observe(merged, [&] (std::string s)
    {
        results.push_back(s);
    });

    std::string s1("one");
    std::string s2("two");
    std::string s3("three");

    DoTransaction<D>([&] {
        a1 << s1;
        a2 << s2;
        a3 << s3;
    });

    ASSERT_EQ(results.size(), 3);
    ASSERT_TRUE(std::find(results.begin(), results.end(), "one") != results.end());
    ASSERT_TRUE(std::find(results.begin(), results.end(), "two") != results.end());
    ASSERT_TRUE(std::find(results.begin(), results.end(), "three") != results.end());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventMerge3 test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(EventStreamTest, EventMerge3)
{
    using D = typename EventMerge3::MyDomain;

    auto a1 = MakeEventSource<D,int>();
    auto a2 = MakeEventSource<D,int>();

    auto f1 = Filter(a1, [] (int v) { return true; });
    auto f2 = Filter(a2, [] (int v) { return true; });

    auto merged = Merge(f1, f2);

    std::queue<int> results;

    Observe(merged, [&] (int s)
    {
        results.push(s);
    });

    a1 << 10;
    a2 << 20;
    a1 << 30;

    ASSERT_FALSE(results.empty());
    ASSERT_EQ(results.front(),10);
    results.pop();

    ASSERT_FALSE(results.empty());
    ASSERT_EQ(results.front(),20);
    results.pop();

    ASSERT_FALSE(results.empty());
    ASSERT_EQ(results.front(),30);
    results.pop();

    ASSERT_TRUE(results.empty());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventFilter test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(EventStreamTest, EventFilter)
{
    using D = typename EventFilter::MyDomain;

    using std::string;

    std::queue<string> results;

    auto in = MakeEventSource<D,string>();

    auto filtered = Filter(in, [] (const string& s)
    {
        return s == "Hello World";
    });


    Observe(filtered, [&] (const string& s)
    {
        results.push(s);
    });

    in << string("Hello Worlt") << string("Hello World") << string("Hello Vorld");

    ASSERT_FALSE(results.empty());
    ASSERT_EQ(results.front(), "Hello World");
    results.pop();

    ASSERT_TRUE(results.empty());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// EventTransform test
///////////////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(EventStreamTest, EventTransform)
{
    using D = typename EventTransform::MyDomain;

    using std::string;

    std::vector<string> results;

    auto in1 = MakeEventSource<D,string>();
    auto in2 = MakeEventSource<D,string>();

    auto merged = Merge(in1, in2);

    auto transformed = Transform(merged, [] (string s) -> string
    {
        std::transform(s.begin(), s.end(), s.begin(), ::toupper);
        return s;
    });

    Observe(transformed, [&] (const string& s)
    {
        results.push_back(s);
    });

    in1 << string("Hello Worlt") << string("Hello World");
    in2 << string("Hello Vorld");

    ASSERT_EQ(results.size(), 3);
    ASSERT_TRUE(std::find(results.begin(), results.end(), "HELLO WORLT") != results.end());
    ASSERT_TRUE(std::find(results.begin(), results.end(), "HELLO WORLD") != results.end());
    ASSERT_TRUE(std::find(results.begin(), results.end(), "HELLO VORLD") != results.end());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
REGISTER_TYPED_TEST_CASE_P
(
    EventStreamTest,
    EventSources,
    EventMerge1,
    EventMerge2,
    EventMerge3,
    EventFilter,
    EventTransform
);

} // ~namespace
