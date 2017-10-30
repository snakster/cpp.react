
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#if 0

#ifndef CPP_REACT_BENCHMARK_FANOUT_H
#define CPP_REACT_BENCHMARK_FANOUT_H

#include <iostream>
#include <random>
#include <vector>

#include "BenchmarkBase.h"

#include "react/state.h"
/*
using namespace react;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Benchmark_Fanout
///////////////////////////////////////////////////////////////////////////////////////////////////
struct BenchmarkParams_Fanout
{
    BenchmarkParams_Fanout(int n, int k, int delay) :
        N(n),
        K(k),
        Delay(delay)
    {}

    void Print(std::ostream& out) const
    {
        out << "N = " << N
            << ", K = " << K
            << ", Delay = " << Delay;
    }

    const int N;
    const int K;
    const int Delay;
};

struct Benchmark_Fanout
{
    double Run(const BenchmarkParams_Fanout& params)
    {
        using MySignal = Signal<D,int>;

        bool initializing = true;

        auto in = MakeVar<D>(1);

        std::vector<MySignal> nodes;
        auto f = [&initializing,&params] (int a)
        {
            if (params.Delay > 0 && !initializing)
            {
                auto t0 = std::chrono::high_resolution_clock::now();
                while (std::chrono::high_resolution_clock::now() - t0 < std::chrono::milliseconds(params.Delay));
            }
            return a + 1;
        };

        for (int i=0; i<params.N; i++)
        {
            auto t = MakeSignal(in, f);
            nodes.push_back(t);
        }

        initializing = false;

        auto t0 = tbb::tick_count::now();
        for (int i=0; i<params.K; i++)
            in <<= 10+i;
        auto t1 = tbb::tick_count::now();

        double d = (t1 - t0).seconds();

        return d;
    }
};

*/

#endif // CPP_REACT_BENCHMARK_FANOUT_H

#endif