#pragma once

#include <queue>
#include <string>

#include "gtest/gtest.h"

#include "react/EventStream.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;

////////////////////////////////////////////////////////////////////////////////////////
/// EventStreamTest fixture
////////////////////////////////////////////////////////////////////////////////////////
template <typename TEngine>
class EventStreamTest : public testing::Test
{
public:
	REACTIVE_DOMAIN(MyDomain, TEngine);
};

TYPED_TEST_CASE_P(EventStreamTest);

////////////////////////////////////////////////////////////////////////////////////////
/// EventSources test
////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(EventStreamTest, EventSources)
{
	auto es1 = MyDomain::MakeEventSource<int>();
	auto es2 = MyDomain::MakeEventSource<int>();

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

////////////////////////////////////////////////////////////////////////////////////////
/// EventMerge1 test
////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(EventStreamTest, EventMerge1)
{
	auto a1 = MyDomain::MakeEventSource<int>();
	auto a2 = MyDomain::MakeEventSource<int>();
	auto a3 = MyDomain::MakeEventSource<int>();

	auto merged = Merge(a1, a2, a3);

	std::queue<int> results;

	Observe(merged, [&] (int v)
	{
		results.push(v);
	});
	
	{
		MyDomain::ScopedTransaction _;
		a1 << 10;
		a2 << 20;
		a3 << 30;
	}

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

////////////////////////////////////////////////////////////////////////////////////////
/// EventMerge2 test
////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(EventStreamTest, EventMerge2)
{
	auto a1 = MyDomain::MakeEventSource<std::string>();
	auto a2 = MyDomain::MakeEventSource<std::string>();
	auto a3 = MyDomain::MakeEventSource<std::string>();

	auto merged = Merge(a1, a2, a3);

	std::queue<std::string> results;

	Observe(merged, [&] (std::string s)
	{
		results.push(s);
	});

	std::string s1("one");
	std::string s2("two");
	std::string s3("three");

	{
		MyDomain::ScopedTransaction t;

		a1 << s1;
		a2 << s2;
		a3 << s3;
	}

	ASSERT_FALSE(results.empty());
	ASSERT_EQ(results.front(), "one");
	results.pop();

	ASSERT_FALSE(results.empty());
	ASSERT_EQ(results.front(), "two");
	results.pop();

	ASSERT_FALSE(results.empty());
	ASSERT_EQ(results.front(), "three");
	results.pop();

	ASSERT_TRUE(results.empty());
}

////////////////////////////////////////////////////////////////////////////////////////
/// EventMerge3 test
////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(EventStreamTest, EventMerge3)
{
	auto a1 = MyDomain::MakeEventSource<int>();
	auto a2 = MyDomain::MakeEventSource<int>();

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

////////////////////////////////////////////////////////////////////////////////////////
/// EventFilter test
////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(EventStreamTest, EventFilter)
{
	std::queue<std::string> results;

	auto in = MyDomain::MakeEventSource<std::string>();

	auto filtered = Filter(in, [] (const std::string& s)
	{
		return s == "Hello World";
	});


	Observe(filtered, [&] (const std::string& s)
	{
		results.push(s);
	});

	in << "Hello Worlt" << "Hello World" << "Hello Vorld";

	ASSERT_FALSE(results.empty());
	ASSERT_EQ(results.front(), "Hello World");
	results.pop();

	ASSERT_TRUE(results.empty());
}

////////////////////////////////////////////////////////////////////////////////////////
/// EventTransform test
////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(EventStreamTest, EventTransform)
{
	std::queue<std::string> results;

	auto in1 = MyDomain::MakeEventSource<std::string>();
	auto in2 = MyDomain::MakeEventSource<std::string>();

	auto merged = Merge(in1, in2);

	auto transformed = Transform(merged, [] (std::string s) -> std::string
	{
		std::transform(s.begin(), s.end(), s.begin(), ::toupper);

		return s;
	});

	int obsCount = 0;

	Observe(transformed, [&] (const std::string& s)
	{
		obsCount++;
		results.push(s);
	});

	in1 << "Hello Worlt" << "Hello World";
	in2 << "Hello Vorld";

	ASSERT_EQ(obsCount, 3);

	ASSERT_FALSE(results.empty());
	ASSERT_EQ(results.front(), "HELLO WORLT");
	results.pop();

	ASSERT_FALSE(results.empty());
	ASSERT_EQ(results.front(), "HELLO WORLD");
	results.pop();

	ASSERT_FALSE(results.empty());
	ASSERT_EQ(results.front(), "HELLO VORLD");
	results.pop();

	ASSERT_TRUE(results.empty());
}

////////////////////////////////////////////////////////////////////////////////////////
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

// ---
}