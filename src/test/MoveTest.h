
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "gtest/gtest.h"

#include "react/Signal.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;

////////////////////////////////////////////////////////////////////////////////////////
/// MoveTest fixture
////////////////////////////////////////////////////////////////////////////////////////
template <typename TEngine>
class MoveTest : public testing::Test
{
public:
	REACTIVE_DOMAIN(MyDomain, TEngine);

	struct Stats
	{
		int copyCount = 0;
		int moveCount = 0;
	};

	struct CopyCounter
	{
		int v = 0;
		Stats* stats = nullptr;

		CopyCounter()
		{	
		}

		CopyCounter(int x, Stats* s) :
			v{ x },
			stats{ s }
		{
		}

		CopyCounter(const CopyCounter& other) :
			v{ other.v },
			stats{ other.stats }
		{
			stats->copyCount++;
		}

		CopyCounter(CopyCounter&& other) :
			v{ other.v },
			stats{ other.stats }
		{
			stats->moveCount++;
		}

		CopyCounter& operator=(const CopyCounter& other)
		{
			v = other.v;
			stats = other.stats;
			stats->copyCount++;
			return *this;
		}

		CopyCounter& operator=(CopyCounter&& other)
		{
			v = other.v;
			stats = other.stats;
			stats->moveCount++;
			return *this;
		}

		CopyCounter operator+(const CopyCounter& r) const
		{
			return CopyCounter{ v + r.v, stats };
		}

		bool operator==(const CopyCounter& other) const
		{
			return v == other.v;
		}
	};
};

TYPED_TEST_CASE_P(MoveTest);

////////////////////////////////////////////////////////////////////////////////////////
/// Copy1
////////////////////////////////////////////////////////////////////////////////////////
TYPED_TEST_P(MoveTest, Copy1)
{
	Stats stats1;

	auto a = MyDomain::MakeVar(CopyCounter{1,&stats1});
	auto b = MyDomain::MakeVar(CopyCounter{10,&stats1});
	auto c = MyDomain::MakeVar(CopyCounter{100,&stats1});
	auto d = MyDomain::MakeVar(CopyCounter{1000,&stats1});

	// 4x move to value_
	// 4x copy to newValue_ (can't be unitialized for references)
	ASSERT_EQ(stats1.copyCount, 4);
	ASSERT_EQ(stats1.moveCount, 4);

	auto x = a + b + c + d;

	ASSERT_EQ(stats1.copyCount, 4);
	ASSERT_EQ(stats1.moveCount, 7);
	ASSERT_EQ(x().v, 1111);

	a <<= CopyCounter{2,&stats1};

	ASSERT_EQ(stats1.copyCount, 4);
	ASSERT_EQ(stats1.moveCount, 12);
	ASSERT_EQ(x().v, 1112);
}

////////////////////////////////////////////////////////////////////////////////////////
REGISTER_TYPED_TEST_CASE_P
(
	MoveTest,
	Copy1
);

} // ~namespace
