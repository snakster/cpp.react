
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#if 0

#ifndef REACT_BENCHMARK_GRID_H
#define REACT_BENCHMARK_GRID_H

#include <functional>
#include <iostream>
#include <vector>

#include "tbb/tick_count.h"

#include "BenchmarkBase.h"

#include "react/state.h"

using namespace react;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// GridGraphGenerator
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
class GridGraphGenerator
{
public:
    using SignalType = Signal<T, shared>;

    using Func1T = std::function<T(T)>;
    using Func2T = std::function<T(T, T)>;

    using SignalVectType = std::vector<SignalType>;

    SignalVectType inputSignals;
    SignalVectType outputSignals;

    Func1T  function1;
    Func2T  function2;

    std::vector<size_t>  widths;

    void Generate(const GroupBase& group)
    {
        assert(inputSignals.size() >= 1);
        assert(widths.size() >= 1);

        SignalVectType buf1 = std::move(inputSignals);
        SignalVectType buf2;

        SignalVectType* curBuf = &buf1;
        SignalVectType* nextBuf = &buf2;

        size_t curWidth = inputSignals.size();

        size_t nodeCount = 1;
        nodeCount += curWidth;

        for (auto targetWidth : widths)
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
										auto s = SignalType{ group, function1, *l };
                    nextBuf->push_back(std::move(s));
                }

                while (r != curBuf->end())
                {
										auto s = SignalType{ group, function2, *l, *r };
                    nextBuf->push_back(std::move(s));
                    ++nodeCount;
                    ++l; ++r;
                }

                if (shouldGrow)
                {
                    auto s = SignalType{ group, function1, *l };
                    nextBuf->push_back(std::move(s));
                    ++nodeCount;
                }

                curBuf->clear();

                // Swap buffer pointers
                SignalVectType* t = curBuf;
                curBuf = nextBuf;
                nextBuf = t;

                if (shouldGrow)
                    ++curWidth;
                else
                    --curWidth;
            }
        }

        //printf ("NODE COUNT %d\n", nodeCount);

        outputSignals.clear();
        outputSignals.insert(outputSignals.begin(), curBuf->begin(), curBuf->end());
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

struct Benchmark_Grid
{
    double Run(const BenchmarkParams_Grid& params, const GroupBase& group)
    {
        VarSignal<int, shared> in{ group, 1 };
				Signal<int, shared> in2 = in;

        GridGraphGenerator<int> generator;

        generator.inputSignals.push_back(in2);

        generator.widths.push_back(params.N);
        generator.widths.push_back(1);

        generator.function1 = [] (int a) { return a; };
        generator.function2 = [] (int a, int b) { return a + b; };

        generator.Generate(group);

        auto t0 = tbb::tick_count::now();
        for (int i=0; i<params.K; i++)
            in <<= 10+i;
        auto t1 = tbb::tick_count::now();

        double d = (t1 - t0).seconds();
        //printf("Time %g\n", d);

        return d;
    }
};

#endif // REACT_BENCHMARK_GRID_H

#endif