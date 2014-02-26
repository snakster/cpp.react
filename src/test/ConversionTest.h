#pragma once

#include <queue>
#include <string>

#include "gtest/gtest.h"

#include "react/Conversion.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;
using namespace std;

////////////////////////////////////////////////////////////////////////////////////////
/// EventStreamTest fixture
////////////////////////////////////////////////////////////////////////////////////////
template <typename TEngine>
class ConversionTest : public testing::Test
{
public:
	REACTIVE_DOMAIN(MyDomain, TEngine);
};

TYPED_TEST_CASE_P(ConversionTest);

////////////////////////////////////////////////////////////////////////////////////////
/// Fold1 test
////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(ConversionTest, Fold1)
{
	auto numSrc = MyDomain::MakeEventSource<int>();
	auto numFold = Fold(0, numSrc, [] (int v, int d) {
		return v + d;
	});

	for (auto i=1; i<=100; i++)
		numSrc << i;

	ASSERT_EQ(numFold(), 5050);

	auto charSrc = MyDomain::MakeEventSource<char>();
	auto strFold = Fold(string(""), charSrc, [] (string s, char c) {
		return s + c;
	});

	charSrc << 'T' << 'e' << 's' << 't';

	ASSERT_EQ(strFold(), "Test");
}

////////////////////////////////////////////////////////////////////////////////////////
/// Fold2 test
////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(ConversionTest, Fold2)
{
	auto src = MyDomain::MakeEventSource<int>();
	auto f = Fold(0, src, [] (int v, int d) {
		return v + d;
	});

	int c = 0;

	Observe(f, [&] (int v) {
		c++;
		ASSERT_EQ(v, 5050);
	});

	{
		MyDomain::ScopedTransaction _;

		for (auto i=1; i<=100; i++)
			src << i;
	}

	ASSERT_EQ(f(), 5050);
	ASSERT_EQ(c, 1);
}

////////////////////////////////////////////////////////////////////////////////////////
/// Iterate1 test
////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(ConversionTest, Iterate1)
{
	auto trigger = MyDomain::MakeEventSource();

	{
		auto inc = Iterate(0, trigger, Incrementer<int>{});
		for (auto i=1; i<=100; i++)
			trigger.Emit();

		ASSERT_EQ(inc(), 100);
	}

	{
		auto dec = Iterate(100, trigger, Decrementer<int>{});
		for (auto i=1; i<=100; i++)
			trigger.Emit();

		ASSERT_EQ(dec(), 0);
	}
}

////////////////////////////////////////////////////////////////////////////////////////
/// Monitor1
////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(ConversionTest, Monitor1)
{
	auto target = MyDomain::MakeSignal();

	{
		auto inc = Iterate(0, trigger, Incrementer<int>{});
		for (auto i=1; i<=100; i++)
			trigger.Emit();

		ASSERT_EQ(inc(), 100);
	}

	{
		auto dec = Iterate(100, trigger, Decrementer<int>{});
		for (auto i=1; i<=100; i++)
			trigger.Emit();

		ASSERT_EQ(dec(), 0);
	}
}

////////////////////////////////////////////////////////////////////////////////////////
REGISTER_TYPED_TEST_CASE_P
(
	ConversionTest,
	Fold1,
	Fold2,
	Iterate1
);

} // ~namespace