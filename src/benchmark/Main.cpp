
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "tbb/tick_count.h"
#include "tbb/tbbmalloc_proxy.h"

#include "BenchmarkGrid.h"
#include "BenchmarkRandom.h"
#include "BenchmarkFanout.h"
#include "BenchmarkSequence.h"
#include "BenchmarkLifeSim.h"

#include "react/Signal.h"
#include "react/Operations.h"
#include "react/common/Util.h"
#include "react/logging/EventLog.h"

#include "react/propagation/FloodingEngine.h"
#include "react/propagation/TopoSortEngine.h"
#include "react/propagation/TopoSortO1Engine.h"
#include "react/propagation/PulseCountEngine.h"
#include "react/propagation/SourceSetEngine.h"
//#include "react/propagation/PulseCountO1Engine.h"
#include "react/propagation/ELMEngine.h"

////////////////////////////////////////////////////////////////////////////////////////
namespace {

using namespace react;

REACTIVE_DOMAIN(FloodingDomain, FloodingEngine<parallel>, EventLog);
REACTIVE_DOMAIN(TopoSortDomain, TopoSortEngine<parallel>, EventLog);
REACTIVE_DOMAIN(PulseCountDomain, PulseCountEngine<parallel>, EventLog);
REACTIVE_DOMAIN(SourceSetDomain, SourceSetEngine<parallel>, EventLog);
REACTIVE_DOMAIN(TopoSortSTDomain, TopoSortEngine<sequential>, EventLog);
REACTIVE_DOMAIN(ELMDomain, ELMEngine<parallel>, EventLog);

REACTIVE_DOMAIN(BFloodingDomain, FloodingEngine<parallel>);
REACTIVE_DOMAIN(BTopoSortDomain, TopoSortEngine<parallel>);
REACTIVE_DOMAIN(BPulseCountDomain, PulseCountEngine<parallel>);
REACTIVE_DOMAIN(BSourceSetDomain, SourceSetEngine<parallel>);
REACTIVE_DOMAIN(BTopoSortSTDomain, TopoSortEngine<sequential>);
REACTIVE_DOMAIN(BELMDomain, ELMEngine<parallel>);

void runBenchmarkGrid(std::ostream& out)
{
	RUN_BENCHMARK(out, 5, Benchmark_Grid, BenchmarkParams_Grid(10, 10000),
		BTopoSortSTDomain, BTopoSortDomain, BELMDomain, BPulseCountDomain, BSourceSetDomain, BFloodingDomain);

	RUN_BENCHMARK(out, 5, Benchmark_Grid, BenchmarkParams_Grid(20, 10000),
		BTopoSortSTDomain, BTopoSortDomain, BELMDomain, BPulseCountDomain, BSourceSetDomain);

	RUN_BENCHMARK(out, 5, Benchmark_Grid, BenchmarkParams_Grid(30, 10000),
		BTopoSortSTDomain, BTopoSortDomain, BELMDomain, BPulseCountDomain, BSourceSetDomain);

	RUN_BENCHMARK(out, 5, Benchmark_Grid, BenchmarkParams_Grid(40, 10000),
		BTopoSortSTDomain, BTopoSortDomain, BELMDomain, BPulseCountDomain, BSourceSetDomain);

	RUN_BENCHMARK(out, 5, Benchmark_Grid, BenchmarkParams_Grid(50, 10000),
		BTopoSortSTDomain, BTopoSortDomain, BELMDomain, BPulseCountDomain, BSourceSetDomain);
}

void runBenchmarkFlooding(std::ostream& out)
{
	RUN_BENCHMARK(out, 5, Benchmark_Grid, BenchmarkParams_Grid(20, 10),
		BFloodingDomain);

	RUN_BENCHMARK(out, 5, Benchmark_Grid, BenchmarkParams_Grid(30, 10),
		BFloodingDomain);

	RUN_BENCHMARK(out, 5, Benchmark_Grid, BenchmarkParams_Grid(40, 10),
		BFloodingDomain);

	RUN_BENCHMARK(out, 5, Benchmark_Grid, BenchmarkParams_Grid(50, 10),
		BFloodingDomain);
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
			//	BTopoSortDomain, BELMDomain, BPulseCountDomain, BSourceSetDomain, BFloodingDomain);

			RUN_BENCHMARK(out, 5, Benchmark_Random, BenchmarkParams_Random(w, h, 20, 0, 10, 40, x, true, seed1, seed2),
				BTopoSortSTDomain);
		}

		seed1 *= 2;
		seed2 *= 2;
	}
}

void runBenchmarkFanout(std::ostream& out)
{
	//RUN_BENCHMARK(out, 5, Benchmark_Fanout, BenchmarkParams_Fanout(10, 10000, 0),
	//	BTopoSortSTDomain, BTopoSortDomain, BELMDomain, BPulseCountDomain, BSourceSetDomain, BFloodingDomain);

	//RUN_BENCHMARK(out, 5, Benchmark_Fanout, BenchmarkParams_Fanout(100, 10000, 0),
	//	BTopoSortSTDomain, BTopoSortDomain, BELMDomain, BPulseCountDomain, BSourceSetDomain, BFloodingDomain);

	//RUN_BENCHMARK(out, 5, Benchmark_Fanout, BenchmarkParams_Fanout(1000, 10000, 0),
	//	BTopoSortSTDomain, BTopoSortDomain, BELMDomain, BPulseCountDomain, BSourceSetDomain, BFloodingDomain);

	RUN_BENCHMARK(out, 3, Benchmark_Fanout, BenchmarkParams_Fanout(10, 10, 10),
		BTopoSortSTDomain, BTopoSortDomain, BELMDomain, BPulseCountDomain, BSourceSetDomain, BFloodingDomain);

	RUN_BENCHMARK(out, 3, Benchmark_Fanout, BenchmarkParams_Fanout(100, 10, 10),
		BTopoSortSTDomain, BTopoSortDomain, BELMDomain, BPulseCountDomain, BSourceSetDomain, BFloodingDomain);

	RUN_BENCHMARK(out, 3, Benchmark_Fanout, BenchmarkParams_Fanout(1000, 10, 10),
		BTopoSortSTDomain, BTopoSortDomain, BELMDomain, BPulseCountDomain, BSourceSetDomain, BFloodingDomain);
}

void runBenchmarkSequence(std::ostream& out)
{
	//RUN_BENCHMARK(out, 3, Benchmark_Sequence, BenchmarkParams_Sequence(10, 10000, 0),
	//	BTopoSortSTDomain, BTopoSortDomain, BELMDomain, BPulseCountDomain, BSourceSetDomain, BFloodingDomain);

	//RUN_BENCHMARK(out, 3, Benchmark_Sequence, BenchmarkParams_Sequence(100, 10000, 0),
	//	BTopoSortSTDomain, BTopoSortDomain, BELMDomain, BPulseCountDomain, BSourceSetDomain, BFloodingDomain);

	//RUN_BENCHMARK(out, 3, Benchmark_Sequence, BenchmarkParams_Sequence(1000, 10000, 0),
	//	BTopoSortSTDomain, BTopoSortDomain, BELMDomain, BPulseCountDomain, BSourceSetDomain, BFloodingDomain);

	RUN_BENCHMARK(out, 3, Benchmark_Sequence, BenchmarkParams_Sequence(10, 10, 10),
		BTopoSortSTDomain, BTopoSortDomain, BELMDomain, BPulseCountDomain, BSourceSetDomain, BFloodingDomain);

	RUN_BENCHMARK(out, 3, Benchmark_Sequence, BenchmarkParams_Sequence(100, 10, 10),
		BTopoSortSTDomain, BTopoSortDomain, BELMDomain, BPulseCountDomain, BSourceSetDomain, BFloodingDomain);

	RUN_BENCHMARK(out, 3, Benchmark_Sequence, BenchmarkParams_Sequence(1000, 10, 10),
		BTopoSortSTDomain, BTopoSortDomain, BELMDomain, BPulseCountDomain, BSourceSetDomain, BFloodingDomain);
}

void runBenchmarkLifeSim(std::ostream& out)
{
	//RUN_BENCHMARK(std::cout, 1, Benchmark_LifeSim, BenchmarkParams_LifeSim(150, 1000),
	//	BELMDomain);

	//RUN_BENCHMARK(std::cout, 1, Benchmark_LifeSim, BenchmarkParams_LifeSim(250, 30, 10000),
	//	BSourceSetDomain, BPulseCountDomain);

	//RUN_BENCHMARK(out, 3, Benchmark_LifeSim, BenchmarkParams_LifeSim(250, 30, 10000),
	//	BTopoSortSTDomain, BTopoSortDomain, BELMDomain, BPulseCountDomain, BPulseCountO1Domain, BSourceSetDomain, BFloodingDomain);

	RUN_BENCHMARK(out, 3, Benchmark_LifeSim, BenchmarkParams_LifeSim(100, 15, 10000),
		BPulseCountDomain, BPulseCountDomain);
}

void runBenchmarks()
{
	std::ofstream logfile;

	std::string path = "Benchmark Results/" + CurrentDateTime() + ".txt";
	logfile.open(path.c_str());

	// === GRID
	runBenchmarkGrid(logfile);

	//runBenchmarkFlooding(logfile);

	// === RANDOM
	//runBenchmarkRandom(logfile);

	// === FANOUT
	//runBenchmarkFanout(logfile);

	// === SEQUENCE
	//runBenchmarkSequence(logfile);

	// === LIFESIM
	//runBenchmarkLifeSim(logfile);

	logfile.close();
}

void debugBenchmarks()
{
	using TestDomain = BPulseCountDomain;

	RUN_BENCHMARK(std::cout, 3, Benchmark_Grid, BenchmarkParams_Grid(30, 1000),
		TestDomain, BPulseCountDomain);

	//RUN_BENCHMARK(std::cout, 1, Benchmark_Fanout, BenchmarkParams_Fanout(1000, 5, 0),
	//	TestDomain);

	//RUN_BENCHMARK(std::cout, 1, Benchmark_Grid, BenchmarkParams_Grid(30, 1),
		//TestDomain);

	//RUN_BENCHMARK(std::cout, 1, Benchmark_Sequence, BenchmarkParams_Sequence(500, 1, 0),
	//	TestDomain);

	//RUN_BENCHMARK(std::cout, 1, Benchmark_Random, BenchmarkParams_Random(10, 25, 10, 0, 10, 25, 25, false),
	//	TopoSortDomain);

	//const auto w = 10;
	//const auto h = 11;

	//for (int slowPercent=0; slowPercent<=50; slowPercent+=50)
	//{
	//	int seed1 = 4156;
	//	int seed2 = 2124;

	//	for (int j=1; j<=5; j++)
	//	{
	//		int x = (slowPercent * (w*(h-1))) / 100;
	//		RUN_BENCHMARK(std::cout, 1, Benchmark_Random, BenchmarkParams_Random(w, h, 100, 0, 5, 25, x, false, seed1 / j, seed2 / j),
	//			TopoSortDomain);
	//	}
	//}

	//RunBenchmark<1>(Benchmark2<BTokenDomain>());
	//RunBenchmark<1>(Benchmark2<BSourceSetDomain>());
	//RunBenchmark<3>(Benchmark2<BTopoSortDomain>());

	//std::ofstream logfile;
	//logfile.open("log.txt");

	//TestDomain::Log().Write(logfile);
	//logfile.close();
}

void profileBenchmark()
{
	RUN_BENCHMARK(std::cout, 3, Benchmark_Grid, BenchmarkParams_Grid(30, 10000),
		BTopoSortDomain);

	//RUN_BENCHMARK(std::cout, 1, Benchmark_Grid, BenchmarkParams_Grid(30, 10000),
	//	BSourceSetDomain);

	//RUN_BENCHMARK(std::cout, 1, Benchmark_Random, BenchmarkParams_Random(20, 11, 100, 0, 10, 40, 40, false, 41556, 21624),
	//	BFloodingDomain);
}

} // ~anonymous namespace 

int main()
{
	//runBenchmarks();
	//debugBenchmarks();	
	profileBenchmark();
}