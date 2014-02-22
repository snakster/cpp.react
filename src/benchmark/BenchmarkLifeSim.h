#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <random>
#include <type_traits>

#include "BenchmarkBase.h"
#include "react/ReactiveDomain.h"
#include "react/ReactiveObject.h"
#include "react/logging/EventLog.h"

using namespace react;

using std::vector;
using std::atomic;
using std::tuple;
using std::unique_ptr;
using std::move;
using std::make_tuple;
using std::cout;
using std::string;
using std::pair;
using std::make_pair;
using std::get;

enum class Seasons		{ summer,  winter };
enum class Migration	{ enter,  leave };

typedef pair<int,int> PositionT;

template <typename TDomain>
class Time : public ReactiveObject<TDomain>
{
public:
	EventSource<bool>	NewDay	= MakeEventSource<bool>();

	Signal<int>		TotalDays	= Iterate(0, NewDay, Incrementer<int>());
	Signal<int>		DayOfYear	= TotalDays % 365;

	Signal<Seasons>	Season = DayOfYear >>= [] (int day) {
		return day < 180 ? Seasons::winter : Seasons::summer;
	};

	~Time()
	{
		DetachAllObservers(NewDay);
	}
};

template <typename TDomain>
class Region : public ReactiveObject<TDomain>
{
	Time<TDomain>&			theTime;

public:
	typedef tuple<int,int,int,int> BoundsT;

	BoundsT Bounds;

	EventSource<Migration>	EnterOrLeave = MakeEventSource<Migration>();

	Signal<int>	AnimalCount = Fold(0, EnterOrLeave, [] (int count, Migration m) {
		return m == Migration::enter ? count + 1 : count - 1;
	});

	Signal<int>	FoodPerDay = theTime.Season >>= [] (Seasons season) {
		return season == Seasons::summer ? 20 : 10;
	};

	Signal<int>	FoodOutputPerDay =
		(FoodPerDay, AnimalCount) >>= [] (int food, int count) {
			return count > 0 ? food/count : 0;
	};

	Events<int> FoodOutput = Pulse(FoodOutputPerDay, theTime.NewDay);

	Region(Time<TDomain>& time, int x, int y):
		theTime { time },
		Bounds  { make_tuple(x*10, x*10+9, y*10, y*10+9) }
	{
	}

	PositionT Center()
	{
		return make_pair(get<0>(Bounds) + 5, get<2>(Bounds) + 5);
	}

	PositionT Clamp(PositionT pos)
	{
		pos.first  = get<0>(Bounds) + (abs(pos.first) % 10);
		pos.second = get<2>(Bounds) + (abs(pos.second) % 10);

		return  pos;
	}

	bool IsInRegion(PositionT pos)
	{
		return  get<0>(Bounds) <= pos.first  && pos.first  <= get<1>(Bounds) &&
				get<2>(Bounds) <= pos.second && pos.second <= get<3>(Bounds);
	}
};

template <typename TDomain>
class World : public ReactiveObject<TDomain>
{
	int w_;

public:
	vector<unique_ptr<Region<TDomain>>>	Regions;

	World(Time<TDomain>& time, int w) :
		w_( w )
	{
		for (int x=0; x<w; x++)
			for (int y=0; y<w; y++)
				Regions.push_back(unique_ptr<Region<TDomain>>(new Region<TDomain>(time, x, y)));
	}

	Region<TDomain>* GetRegion(PositionT pos)
	{
		for (auto& r : Regions)
		{
			if (r->IsInRegion(pos))
				return r.get();
		}

		printf("FATAL ERROR %d %d\n", pos.first, pos.second);
		return nullptr;
	}

	PositionT Clamp(PositionT pos)
	{
		pos.first = abs(pos.first) % 10*w_;
		pos.second = abs(pos.second) % 10*w_;

		return  pos;
	}
};

template <typename TDomain>
class Animal : public ReactiveObject<TDomain>
{
	Time<TDomain>&	theTime;
	World<TDomain>&	theWorld;

	std::mt19937 generator;

public:
	Signal<PositionT>			Position;
	VarSignal<Region<TDomain>*>	CurrentRegion;
	Signal<Region<TDomain>*>	NewRegion;

	Animal(Time<TDomain>& time, World<TDomain>&	world, Region<TDomain>* initRegion, unsigned seed) :
		theTime( time ),
		theWorld( world ),
		CurrentRegion( MakeVar(initRegion) ),
		generator( seed )
	{
		Position = Fold(initRegion->Center(), Moving, [this] (PositionT position, bool shouldMigrate) {
			std::uniform_int_distribution<int> dist(-1,1);

			// Wander randomly
			for (int i=0; i<100; i++)
			{
				position.first  += dist(generator);
				position.second += dist(generator);
			}

			if (shouldMigrate)
				return theWorld.Clamp(position);
			else
				return CurrentRegion.Value()->Clamp(position);
		});

		NewRegion = (Position) >>= [this] (PositionT pos)
		{
			return theWorld.GetRegion(pos);
		};

		initRegion->EnterOrLeave << Migration::enter;

		Observe(NewRegion, [this] (Region<TDomain>* newRegion) {
			CurrentRegion.Value()->EnterOrLeave << Migration::leave;
			newRegion->EnterOrLeave << Migration::enter;

			CurrentRegion <<= newRegion;

			//Migrating << true;
		});
	}
		
	Events<int> FoodReceived = DYNAMIC_REF(CurrentRegion, FoodOutput);

	Signal<int>	Age = Iterate(0, theTime.NewDay, Incrementer<int>());

	Signal<int>	Health = Fold(100, FoodReceived, [] (int health, int food) {
		auto newHealth = health + food - 10;
		return newHealth < 0 ? 0 : newHealth > 10000 ? 10000 : newHealth;
	});

	//Signal<bool>		YoungAndHealthy = Age < 1000 && Health > 0;
	
	//Event<bool>		Migrating = EventSource<bool>();

	Signal<bool>		ShouldMigrate = Hold(0, FoodReceived) < 10;
	Events<bool>		Moving = Pulse(ShouldMigrate, theTime.NewDay);
};

////////////////////////////////////////////////////////////////////////////////////////
/// Run simulation
////////////////////////////////////////////////////////////////////////////////////////
struct BenchmarkParams_LifeSim
{
	BenchmarkParams_LifeSim(int n, int w, int k) :
		N( n ),
		W( w ),
		K( k )
	{
	}

	const int N;
	const int W;
	const int K;

	void Print(std::ostream& out) const
	{
		out << "N = " << N
			<< ", K = " << K
			<< ", W = " << W;
	}
};

template <typename TDomain>
struct Benchmark_LifeSim : public BenchmarkBase<TDomain>
{
	double Run(const BenchmarkParams_LifeSim& params)
	{
		auto theTime	= Time<TDomain>();
		auto theWorld	= World<TDomain>(theTime, params.W);

		auto animals	= vector<unique_ptr<Animal<TDomain>>>();

		std::mt19937 gen(2015);
		std::uniform_int_distribution<int> dist(0,theWorld.Regions.size()-1);

		for (int i=0; i<params.N; i++)
		{
			auto r = theWorld.Regions[dist(gen)].get();
			animals.push_back(unique_ptr<Animal<TDomain>>(new Animal<TDomain>(theTime, theWorld, r, i+1)));
		}

		atomic<int> c(0);
		int s = 0;

		//for (const auto& e : animals)
		//{
		//	Observe(e->Migrating, [&c] (bool _) {
		//		c++;
		//	});
		//}

		//Observe(theEnvironment.SummerIsComing, [] (int v) {
		//	printf("=== SUMMER ===\n");
		//});

		//Observe(theEnvironment.WinterIsComing, [] (int v) {
		//	printf("=== WINTER ===\n");
		//});

		// WHEEL IN THE SKY KEEPS ON TURNING
		auto t0 = tbb::tick_count::now();
		for (int i=0; i<params.K; i++)
		{
			theTime.NewDay << true;
			
			
			//s = s + c;
			//printf("c %d\n", c);	
			//c = 0;
		}
		auto t1 = tbb::tick_count::now();
		double d = (t1 - t0).seconds();

		//for (const auto& e : animals)
		//{
		//	DetachAllObservers(e->Migrating);
		//}

		//s = s / params.K;
		//printf("s %d\n", s);

		s = s / params.K;
		printf("s/k %d\n", s);

/*		std::ofstream logfile;
		logfile.open("log.txt");

		TDomain::Log().Write(logfile);
		logfile.close()*/;

		return d;
	}
};