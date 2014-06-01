
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <functional>
#include <iostream>
#include <vector>

#include "tbb/tick_count.h"

#include "BenchmarkBase.h"

#include "react/Signal.h"

using namespace react;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// GridGraphGenerator
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename TValue
>
class GridGraphGenerator
{
public:
    using MySignal = Signal<D,TValue>;

    using Func1T = std::function<TValue(TValue)>;
    using Func2T = std::function<TValue(TValue,TValue)>;

    using SignalVectT   = std::vector<MySignal>;
    using WidthVectT    = std::vector<int>;

    SignalVectT InputSignals;
    SignalVectT OutputSignals;

    Func1T  Function1;
    Func2T  Function2;

    WidthVectT  Widths;

    void Generate()
    {
        assert(InputSignals.size() >= 1);
        assert(Widths.size() >= 1);

        SignalVectT buf1 = InputSignals;
        SignalVectT buf2;

        SignalVectT* curBuf = &buf1;
        SignalVectT* nextBuf = &buf2;

        int curWidth = InputSignals.size();

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
                    auto s = (*l) ->* Function1;
                    nextBuf->push_back(s);
                }

                while (r != curBuf->end())
                {
                    auto s = (*l,*r) ->* Function2;
                    nextBuf->push_back(s);
                    nodeCount++;
                    ++l; ++r;
                }

                if (shouldGrow)
                {
                    auto s = (*l) ->* Function1;
                    nextBuf->push_back(s);
                    nodeCount++;
                }

                curBuf->clear();

                // Swap buffer pointers
                SignalVectT* t = curBuf;
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

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Benchmark_Grid
///////////////////////////////////////////////////////////////////////////////////////////////////
struct BenchmarkParams_Grid
{
    BenchmarkParams_Grid(int n, int k) :
        N(n),
        K(k)
    {}

    void Print(std::ostream& out) const
    {
        out << "N = " << N
            << ", K = " << K;
    }

    const int N;
    const int K;
};

template <typename D>
struct Benchmark_Grid : public BenchmarkBase<D>
{
    double Run(const BenchmarkParams_Grid& params)
    {
        auto in = MakeVar<D>(1);

        GridGraphGenerator<D,int> generator;

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