
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#if 0

#include <functional>
#include <iostream>
#include <random>
#include <vector>

#include "BenchmarkBase.h"

#include "react/group.h"
#include "react/state.h"

using namespace react;
/*
///////////////////////////////////////////////////////////////////////////////////////////////////
/// DiamondGraphGenerator
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename D,
    typename TValue
>
class RandomGraphGenerator
{
public:
    typedef typename D::template SignalT<TValue>        MyHandle;
    typedef typename D::template VarSignalT<TValue>     MyInputHandle;

    typedef std::vector<MyHandle>       HandleVect;
    typedef std::vector<MyInputHandle>  InputHandleVect;

    InputHandleVect InputSignals;
    HandleVect      OutputSignals;

    std::function<void()>    SlowDelayFunc;
    std::function<void()>    FastDelayFunc;


    typedef std::function<TValue(TValue)>                       Func1T;
    typedef std::function<TValue(TValue,TValue)>                Func2T;
    typedef std::function<TValue(TValue,TValue,TValue)>         Func3T;
    typedef std::function<TValue(TValue,TValue,TValue,TValue)>  Func4T;

    Func1T    Function1;
    Func2T    Function2;
    Func3T    Function3;
    Func4T    Function4;

    int     Width    = 1;
    int     Height    = 1;

    int     SelectSlowCount = 0;
    int     SelectEdgeCount = 0;

    int     EdgeSeed;
    int     SlowSeed;

    void Generate()
    {
        assert(InputSignals.size() == Width);

        Func1T f1Slow = [this] (TValue a1)                                  { SlowDelayFunc(); return Function1(a1); };
        Func2T f2Slow = [this] (TValue a1, TValue a2)                       { SlowDelayFunc(); return Function2(a1,a2); };
        Func3T f3Slow = [this] (TValue a1, TValue a2, TValue a3)            { SlowDelayFunc(); return Function3(a1,a2,a3); };
        Func4T f4Slow = [this] (TValue a1, TValue a2, TValue a3, TValue a4) { SlowDelayFunc(); return Function4(a1,a2,a3,a4); };

        Func1T f1Fast = [this] (TValue a1)                                  { FastDelayFunc(); return Function1(a1); };
        Func2T f2Fast = [this] (TValue a1, TValue a2)                       { FastDelayFunc(); return Function2(a1,a2); };
        Func3T f3Fast = [this] (TValue a1, TValue a2, TValue a3)            { FastDelayFunc(); return Function3(a1,a2,a3); };
        Func4T f4Fast = [this] (TValue a1, TValue a2, TValue a3, TValue a4) { FastDelayFunc(); return Function4(a1,a2,a3,a4); };

        int nodeCount = Width * Height;
        
        std::mt19937 edgeGen(EdgeSeed);
        const auto edgeNodes = GetUniqueRandomNumbers(edgeGen, Width, nodeCount - 1, SelectEdgeCount);

        std::mt19937 slowGen(SlowSeed);
        const auto slowNodes = GetUniqueRandomNumbers(slowGen, Width, nodeCount - 1, SelectSlowCount);

        std::mt19937 edgeCountGen(EdgeSeed);
        std::geometric_distribution<int> edgeCountDist(0.5);

        auto edgeNodeIt = edgeNodes.begin();
        auto slowNodeIt = slowNodes.begin();

        auto cur = 0;
        HandleVect nodes(nodeCount);

        for (int w=0; w<Width; w++)
            nodes[cur++] = InputSignals[w];

        for (int h=1; h<Height; h++)
        {
            std::uniform_int_distribution<int> nodeDist(0, Width*h - 1);

            for (int w=0; w<Width; w++)
            {
                Func1T f1 = f1Fast;
                Func2T f2 = f2Fast;
                Func3T f3 = f3Fast;
                Func4T f4 = f4Fast;

                // Delay
                if (slowNodeIt != slowNodes.end() && cur == *slowNodeIt)
                {
                    ++slowNodeIt;

                    f1 = f1Slow;
                    f2 = f2Slow;
                    f3 = f3Slow;
                    f4 = f4Slow;
                }

                // Edges
                if (edgeNodeIt != edgeNodes.end() && cur == *edgeNodeIt)
                {
                    ++edgeNodeIt;

                    int edgeCount;
                    do
                        edgeCount = 2 + edgeCountDist(edgeCountGen);
                    while (edgeCount > 4);

                    int kNode0 = cur-Width;

                    int rNode1 = nodeDist(edgeGen);
                    while (rNode1 == kNode0)
                        rNode1 = nodeDist(edgeGen);

                    int rNode2 = nodeDist(edgeGen);
                    while (rNode2 == kNode0 || rNode2 == rNode1)
                        rNode2 = nodeDist(edgeGen);

                    int rNode3 = nodeDist(edgeGen);
                    while (rNode3 == kNode0 || rNode3 == rNode2 || rNode3 == rNode1)
                        rNode3 = nodeDist(edgeGen);

                    if (edgeCount == 2)
                    {
                        nodes[cur] = (nodes[kNode0], nodes[rNode1]) ->* f2;
                    }
                    else if (edgeCount == 3)
                    {
                        nodes[cur] = (nodes[kNode0], nodes[rNode1], nodes[rNode2]) ->* f3;
                    }
                    else
                    {
                        nodes[cur] = (nodes[kNode0], nodes[rNode1], nodes[rNode2], nodes[rNode3]) ->* f4;
                    }
                }
                else
                {
                    nodes[cur] = nodes[cur-Width] ->* f1;
                }

                cur++;
            }
        }

        OutputSignals.clear();
        for (int i=Width*(Height-1); i<nodeCount; i++)
            OutputSignals.push_back(nodes[i]);
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Benchmark_Random
///////////////////////////////////////////////////////////////////////////////////////////////////
struct BenchmarkParams_Random
{
    BenchmarkParams_Random(int w, int h, int k, int fastDelay, int slowDelay, int edgeCount, int slowCount, bool randInput, int edgeSeed = 2014, int slowSeed = 46831) :
        W(w),
        H(h),
        K(k),
        FastDelay(fastDelay),
        SlowDelay(slowDelay),
        EdgeCount(edgeCount),
        SlowCount(slowCount),
        WithRandomInput(randInput),
        EdgeSeed(edgeSeed),
        SlowSeed(slowSeed)
    {}

    void Print(std::ostream& out) const
    {
        out << "W = " << W
            << ", H = " << H
            << ", K = " << K
            << ", FastDelay = " << FastDelay
            << ", SlowDelay = " << SlowDelay
            << ", EdgeCount = " << EdgeCount
            << ", SlowCount = " << SlowCount
            << ", WithRandomInput = " << WithRandomInput
            << ", EdgeSeed = " << EdgeSeed
            << ", SlowSeed = " << SlowSeed;
    }

    const int W;
    const int H;
    const int K;
    const int FastDelay;
    const int SlowDelay;
    const int EdgeCount;
    const int SlowCount;
    const bool WithRandomInput;

    const int EdgeSeed;
    const int SlowSeed;
};

struct Benchmark_Random
{
    double Run(const BenchmarkParams_Random& params)
    {
        RandomGraphGenerator<D,int> generator;

        for (int i=0; i<params.W; i++)
            generator.InputSignals.push_back(MakeVar<D>(1));

        generator.Width = params.W;
        generator.Height = params.H;

        generator.EdgeSeed = params.EdgeSeed;
        generator.SlowSeed = params.SlowSeed;

        generator.Function1 = [] (int a1)                            { return a1; };
        generator.Function2 = [] (int a1, int a2)                    { return a1 + a2; };
        generator.Function3 = [] (int a1, int a2, int a3)            { return a1 + a2 + a3; };
        generator.Function4 = [] (int a1, int a2, int a3, int a4)    { return a1 + a2 + a3 + a4; };

        bool initializing = true;

        if (params.FastDelay > 0)
        {
            generator.FastDelayFunc = [&initializing, &params]
            {
                if (!initializing)
                {
                    auto t0 = std::chrono::high_resolution_clock::now();
                    while (std::chrono::high_resolution_clock::now() - t0 < std::chrono::milliseconds(params.FastDelay));
                }
            };
        }
        else
        {
            generator.FastDelayFunc = [] {};
        }

        if (params.SlowDelay > 0)
        {
            generator.SlowDelayFunc = [&initializing, &params]
            {
                if (!initializing)
                {
                    auto t0 = std::chrono::high_resolution_clock::now();
                    while (std::chrono::high_resolution_clock::now() - t0 < std::chrono::milliseconds(params.SlowDelay));
                }
            };
        }
        else
        {
            generator.SlowDelayFunc = [] {};
        }

        generator.SelectEdgeCount = params.EdgeCount;
        generator.SelectSlowCount = params.SlowCount;

        generator.Generate();

        initializing = false;

        auto counts = std::vector<int>(params.K);

        if (params.WithRandomInput)
        {
            std::mt19937 gen(2015);
            std::geometric_distribution<int> inputDist(0.5);

            // Pre-gen count of changed nodes per iteration
            for (int i=0; i<params.K; i++)
            {
                int x;
                do
                    x = 1 + inputDist(gen);
                while (x > params.W);
                counts[i] = x;
            }
        }
        else
        {
            for (int i=0; i<params.K; i++)
                counts[i] = params.W;
        }

        int cursor = 0;

        auto t0 = tbb::tick_count::now();
        for (int i=0; i<params.K; i++)
        {
            DoTransaction<D>([&] {
                for (int j=0; j<counts[i]; j++)
                {
                    generator.InputSignals[cursor++] <<= 10+i;

                    if (cursor >= params.W)
                        cursor = 0;
                }
            });
        }
        auto t1 = tbb::tick_count::now();

        double d = (t1 - t0).seconds();

        return d;
    }
};*/

#endif