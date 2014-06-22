
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//#define REACT_ENABLE_LOGGING

#include "tbb/tick_count.h"
#include "tbb/tbbmalloc_proxy.h"

//#include "ittnotify.h"

#include "BenchmarkGrid.h"
#include "BenchmarkRandom.h"
#include "BenchmarkFanout.h"
#include "BenchmarkSequence.h"
#include "BenchmarkLifeSim.h"

#include "react/Domain.h"
#include "react/Signal.h"
#include "react/Algorithm.h"
#include "react/common/Util.h"
#include "react/logging/EventLog.h"

#include "react/engine/ToposortEngine.h"
#include "react/engine/PulsecountEngine.h"
#include "react/engine/SubtreeEngine.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;

REACTIVE_DOMAIN(ToposortSTDomain, sequential, ToposortEngine)
REACTIVE_DOMAIN(ToposortDomain, parallel, ToposortEngine)
REACTIVE_DOMAIN(PulsecountDomain, parallel, PulsecountEngine)
REACTIVE_DOMAIN(SubtreeDomain, parallel, SubtreeEngine)

void runBenchmarkGrid(std::ostream& out)
{
    RUN_BENCHMARK(out, 5, Benchmark_Grid, BenchmarkParams_Grid(20, 10000),
        ToposortSTDomain, ToposortDomain, PulsecountDomain);

    RUN_BENCHMARK(out, 5, Benchmark_Grid, BenchmarkParams_Grid(30, 10000),
        ToposortSTDomain, ToposortDomain, PulsecountDomain);

    RUN_BENCHMARK(out, 5, Benchmark_Grid, BenchmarkParams_Grid(40, 10000),
        ToposortSTDomain, ToposortDomain, PulsecountDomain);

    RUN_BENCHMARK(out, 5, Benchmark_Grid, BenchmarkParams_Grid(50, 10000),
        ToposortSTDomain, ToposortDomain, PulsecountDomain);
}

void runBenchmarkRandom(std::ostream& out)
{
    const auto w = 20;
    const auto h = 11;

    int seed1 = 41556;
    int seed2 = 21624;

    for (int j=1; j<=10; j++)
    {
        printf("STARTING SERIES %d\n", j);

        for (int slowPercent=0; slowPercent<=50; slowPercent+=5)
        {
            int x = (slowPercent * (w*(h-1))) / 100;
            //RUN_BENCHMARK(out, 5, Benchmark_Random, BenchmarkParams_Random(w, h, 20, 0, 10, 40, x, true, seed1, seed2),
            //    ToposortDomain, ELMDomain, PulsecountDomain, SourceSetDomain);

            RUN_BENCHMARK(out, 5, Benchmark_Random, BenchmarkParams_Random(w, h, 20, 0, 10, 40, x, true, seed1, seed2),
                ToposortSTDomain);
        }

        seed1 *= 2;
        seed2 *= 2;
    }
}

void runBenchmarkFanout(std::ostream& out)
{
    //RUN_BENCHMARK(out, 5, Benchmark_Fanout, BenchmarkParams_Fanout(10, 10000, 0),
    //    ToposortSTDomain, ToposortDomain, ELMDomain, PulsecountDomain, SourceSetDomain);

    //RUN_BENCHMARK(out, 5, Benchmark_Fanout, BenchmarkParams_Fanout(100, 10000, 0),
    //    ToposortSTDomain, ToposortDomain, ELMDomain, PulsecountDomain, SourceSetDomain);

    //RUN_BENCHMARK(out, 5, Benchmark_Fanout, BenchmarkParams_Fanout(1000, 10000, 0),
    //    ToposortSTDomain, ToposortDomain, ELMDomain, PulsecountDomain, SourceSetDomain);

    RUN_BENCHMARK(out, 3, Benchmark_Fanout, BenchmarkParams_Fanout(10, 10, 10),
        ToposortSTDomain, ToposortDomain, PulsecountDomain);

    RUN_BENCHMARK(out, 3, Benchmark_Fanout, BenchmarkParams_Fanout(100, 10, 10),
        ToposortSTDomain, ToposortDomain, PulsecountDomain);

    RUN_BENCHMARK(out, 3, Benchmark_Fanout, BenchmarkParams_Fanout(1000, 10, 10),
        ToposortSTDomain, ToposortDomain, PulsecountDomain);
}

void runBenchmarkSequence(std::ostream& out)
{
    //RUN_BENCHMARK(out, 3, Benchmark_Sequence, BenchmarkParams_Sequence(10, 10000, 0),
    //    ToposortSTDomain, ToposortDomain, ELMDomain, PulsecountDomain, SourceSetDomain);

    //RUN_BENCHMARK(out, 3, Benchmark_Sequence, BenchmarkParams_Sequence(100, 10000, 0),
    //    ToposortSTDomain, ToposortDomain, ELMDomain, PulsecountDomain, SourceSetDomain);

    //RUN_BENCHMARK(out, 3, Benchmark_Sequence, BenchmarkParams_Sequence(1000, 10000, 0),
    //    ToposortSTDomain, ToposortDomain, ELMDomain, PulsecountDomain, SourceSetDomain);

    RUN_BENCHMARK(out, 3, Benchmark_Sequence, BenchmarkParams_Sequence(10, 10, 10),
        ToposortSTDomain, ToposortDomain, PulsecountDomain);

    RUN_BENCHMARK(out, 3, Benchmark_Sequence, BenchmarkParams_Sequence(100, 10, 10),
        ToposortSTDomain, ToposortDomain, PulsecountDomain);

    RUN_BENCHMARK(out, 3, Benchmark_Sequence, BenchmarkParams_Sequence(1000, 10, 10),
        ToposortSTDomain, ToposortDomain, PulsecountDomain);
}

void runBenchmarkLifeSim(std::ostream& out)
{
    //RUN_BENCHMARK(std::cout, 1, Benchmark_LifeSim, BenchmarkParams_LifeSim(150, 1000),
    //    ELMDomain);

    //RUN_BENCHMARK(std::cout, 1, Benchmark_LifeSim, BenchmarkParams_LifeSim(250, 30, 10000),
    //    SourceSetDomain, PulsecountDomain);

    RUN_BENCHMARK(out, 1, Benchmark_LifeSim, BenchmarkParams_LifeSim(100, 15, 10000),
        ToposortSTDomain, ToposortDomain, PulsecountDomain);

    //RUN_BENCHMARK(out, 3, Benchmark_LifeSim, BenchmarkParams_LifeSim(100, 50, 100),
    //    PulsecountDomain, PulsecountDomain);
}

void runBenchmarks()
{
    std::ofstream logfile;

    std::string path = "Benchmark Results/" + CurrentDateTime() + ".txt";
    logfile.open(path.c_str());

    // === GRID
    //runBenchmarkGrid(logfile);

    //runBenchmarkFlooding(logfile);

    // === RANDOM
    //runBenchmarkRandom(logfile);

    // === FANOUT
    //runBenchmarkFanout(logfile);

    // === SEQUENCE
    //runBenchmarkSequence(logfile);

    // === LIFESIM
    runBenchmarkLifeSim(logfile);

    logfile.close();
}

void debugBenchmarks()
{
    using TestDomain = PulsecountDomain;

    //RUN_BENCHMARK(std::cout, 1, Benchmark_Grid, BenchmarkParams_Grid(30, 1),
    //    TestDomain);

    //RUN_BENCHMARK(std::cout, 1, Benchmark_Fanout, BenchmarkParams_Fanout(1000, 5, 0),
    //    TestDomain);

    //RUN_BENCHMARK(std::cout, 1, Benchmark_Grid, BenchmarkParams_Grid(30, 1),
        //TestDomain);

    //RUN_BENCHMARK(std::cout, 1, Benchmark_Sequence, BenchmarkParams_Sequence(500, 1, 0),
    //    TestDomain);

    //RUN_BENCHMARK(std::cout, 1, Benchmark_Random, BenchmarkParams_Random(10, 25, 10, 0, 10, 25, 25, false),
    //    ToposortDomain);

    RUN_BENCHMARK(std::cout, 1, Benchmark_Random, BenchmarkParams_Random(40, 11, 2, 0, 1, 40, 40, false, 41556, 21624),
        TestDomain);

    //const auto w = 10;
    //const auto h = 11;

    //for (int slowPercent=0; slowPercent<=50; slowPercent+=50)
    //{
    //    int seed1 = 4156;
    //    int seed2 = 2124;

    //    for (int j=1; j<=5; j++)
    //    {
    //        int x = (slowPercent * (w*(h-1))) / 100;
    //        RUN_BENCHMARK(std::cout, 1, Benchmark_Random, BenchmarkParams_Random(w, h, 100, 0, 5, 25, x, false, seed1 / j, seed2 / j),
    //            ToposortDomain);
    //    }
    //}

    //RunBenchmark<1>(Benchmark2<BTokenDomain>());
    //RunBenchmark<1>(Benchmark2<SourceSetDomain>());
    //RunBenchmark<3>(Benchmark2<ToposortDomain>());

#ifdef REACT_ENABLE_LOGGING
    std::ofstream logfile;
    logfile.open("log.txt");

    TestDomain::Log().Write(logfile);
    logfile.close();
#endif
}

void profileBenchmark()
{
    RUN_BENCHMARK(std::cout, 1, Benchmark_Grid, BenchmarkParams_Grid(30, 10000),
        ToposortSTDomain);
        //ToposortSTDomain, ToposortDomain, PulsecountDomain, SubtreeDomain);

    //RUN_BENCHMARK(std::cout, 1, Benchmark_Grid, BenchmarkParams_Grid(30, 10000),
        //SourceSetDomain);

    //RUN_BENCHMARK(std::cout, 3, Benchmark_Random, BenchmarkParams_Random(20, 11, 100, 0, 1, 40, 40, false, 41556, 21624),
        //SubtreeDomain);
        //ToposortSTDomain, ToposortDomain, PulsecountDomain, SubtreeDomain);

    //RUN_BENCHMARK(std::cout, 1, Benchmark_LifeSim, BenchmarkParams_LifeSim(100, 15, 10000),
        //ToposortSTDomain);
}

} // ~anonymous namespace 

int main()
{
    //runBenchmarks();
    //debugBenchmarks();    
    profileBenchmark();
}