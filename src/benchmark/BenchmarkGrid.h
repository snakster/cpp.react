#pragma once

#include <functional>
#include <iostream>
#include <random>
#include <vector>

#include "BenchmarkBase.h"

////////////////////////////////////////////////////////////////////////////////////////
/// GridGraphGenerator
////////////////////////////////////////////////////////////////////////////////////////
template
<
	typename D,
	typename TValue
>
class GridGraphGenerator
{
public:
	typedef D::Signal<TValue>	MyHandle;

	typedef std::function<TValue(TValue)>			Func1T;
	typedef std::function<TValue(TValue,TValue)>	Func2T;

	typedef std::vector<MyHandle>	HandleVect;
	typedef std::vector<int>		WidthVect;

	HandleVect	InputSignals;
	HandleVect	OutputSignals;

	Func1T	Function1;
	Func2T	Function2;

	WidthVect	Widths;

	void Generate()
	{
		assert(InputSignals.size() >= 1);
		assert(Widths.size() >= 1);

		HandleVect buf1 = InputSignals;
		HandleVect buf2;

		HandleVect* curBuf = &buf1;
		HandleVect* nextBuf = &buf2;

		uint curWidth = InputSignals.size();

		int nodeCount = 1;
		nodeCount += curWidth;

		for (auto targetWidth : Widths)
		{
			while (curWidth != targetWidth)
			{
				// Grow or shrink?
				bool shouldGrow = targetWidth > curWidth;

				auto l = curBuf->begin();
				auto r = curBuf->begin();
				if (r != curBuf->end())
					++r;

				if (shouldGrow)
				{
					auto s = (*l) >>= Function1;
					nextBuf->push_back(s);
				}

				while (r != curBuf->end())
				{
					auto s = (*l,*r) >>= Function2;
					nextBuf->push_back(s);
					nodeCount++;
					++l; ++r;
				}

				if (shouldGrow)
				{
					auto s = (*l) >>= Function1;
					nextBuf->push_back(s);
					nodeCount++;
				}

				curBuf->clear();

				// Swap buffer pointers
				HandleVect* t = curBuf;
				curBuf = nextBuf;
				nextBuf = t;

				if (shouldGrow)
					curWidth++;
				else
					curWidth--;
			}
		}

		//printf ("NODE COUNT %d\n", nodeCount);

		OutputSignals.clear();
		OutputSignals.insert(OutputSignals.begin(), curBuf->begin(), curBuf->end());
	}
};

////////////////////////////////////////////////////////////////////////////////////////
/// Benchmark_Grid
////////////////////////////////////////////////////////////////////////////////////////
struct BenchmarkParams_Grid
{
	BenchmarkParams_Grid(int n, int k) :
		N(n),
		K(k)
	{
	}

	const int N;
	const int K;

	void Print(std::ostream& out) const
	{
		out << "N = " << N
			<< ", K = " << K;
	}
};

template <typename D>
struct Benchmark_Grid : public BenchmarkBase<D>
{
	double Run(const BenchmarkParams_Grid& params)
	{
		using MyDomain = D;

		auto in = MyDomain::MakeVar(1);

		GridGraphGenerator<MyDomain,int> generator;

		generator.InputSignals.push_back(in);

		generator.Widths.push_back(params.N);
		generator.Widths.push_back(1);

		generator.Function1 = [] (int a) { return a; };
		generator.Function2 = [] (int a, int b) { return a + b; };

		generator.Generate();

		auto t0 = tbb::tick_count::now();
		for (int i=0; i<params.K; i++)
			in <<= 10+i;
		auto t1 = tbb::tick_count::now();

		double d = (t1 - t0).seconds();
		//printf("Time %g\n", d);

		return d;
	}
};