
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <atomic>
#include <iostream>
#include <fstream>
#include <memory>
#include <random>
#include <utility>
#include <type_traits>

#include "BenchmarkBase.h"
#include "react/ReactiveObject.h"
#include "react/logging/EventLog.h"

using namespace react;

using std::atomic;
using std::vector;

using std::tuple;
using std::unique_ptr;
using std::move;
using std::make_tuple;
using std::cout;
using std::string;
using std::pair;
using std::make_pair;
using std::get;

enum class Seasons      { summer,  winter };
enum class Migration    { enter,  leave };

typedef pair<int,int> PositionT;

template <typename D>
class Time : public ReactiveObject<D>
{
public:
    EventSourceT<bool>    NewDay    = MakeEventSource<bool>();

    SignalT<int>        TotalDays    = Iterate(0, NewDay, Incrementer<int>());
    SignalT<int>        DayOfYear    = TotalDays % 365;

    SignalT<Seasons>    Season = DayOfYear ->* [] (int day) {
        return day < 180 ? Seasons::winter : Seasons::summer;
    };
};

template <typename D>
class Region : public ReactiveObject<D>
{
    Time<D>&            theTime;

public:
    using BoundsT = tuple<int,int,int,int>;

    BoundsT Bounds;

    EventSourceT<Migration>    EnterOrLeave = MakeEventSource<Migration>();

    SignalT<int>    AnimalCount = Fold(0, EnterOrLeave, [] (int count, Migration m) {
        return m == Migration::enter ? count + 1 : count - 1;
    });

    SignalT<int>    FoodPerDay = theTime.Season ->* [] (Seasons season) {
        return season == Seasons::summer ? 20 : 10;
    };

    SignalT<int>    FoodOutputPerDay =
        (FoodPerDay, AnimalCount) ->* [] (int food, int count) {
            return count > 0 ? food/count : 0;
    };

    EventsT<int> FoodOutput = Pulse(FoodOutputPerDay, theTime.NewDay);

    Region(Time<D>& time, int x, int y):
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

template <typename D>
class World : public ReactiveObject<D>
{
    int w_;

public:
    vector<unique_ptr<Region<D>>>    Regions;

    World(Time<D>& time, int w) :
        w_( w )
    {
        for (int x=0; x<w; x++)
            for (int y=0; y<w; y++)
                Regions.push_back(unique_ptr<Region<D>>(new Region<D>(time, x, y)));
    }

    Region<D>* GetRegion(PositionT pos)
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

template <typename D>
class Animal : public ReactiveObject<D>
{
    Time<D>&    theTime;
    World<D>&    theWorld;

    std::mt19937 generator;

public:
    SignalT<PositionT>      Position;
    VarSignalT<Region<D>*>  CurrentRegion;
    SignalT<Region<D>*>     NewRegion;

    Animal(Time<D>& time, World<D>& world, Region<D>* initRegion, unsigned seed) :
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

        NewRegion = (Position) ->* [this] (PositionT pos)
        {
            return theWorld.GetRegion(pos);
        };

        initRegion->EnterOrLeave << Migration::enter;

        Observe(NewRegion, [this] (Region<D>* newRegion) {
            CurrentRegion.Value()->EnterOrLeave << Migration::leave;
            newRegion->EnterOrLeave << Migration::enter;

            CurrentRegion <<= newRegion;

            //Migrating << true;
        });
    }
        
    EventsT<int>    FoodReceived = REACTIVE_PTR(CurrentRegion, FoodOutput);

    SignalT<int>    Age = Iterate(0, theTime.NewDay, Incrementer<int>());

    SignalT<int>    Health = Fold(100, FoodReceived, [] (int health, int food) {
        auto newHealth = health + food - 10;
        return newHealth < 0 ? 0 : newHealth > 10000 ? 10000 : newHealth;
    });

    //Signal<bool>  YoungAndHealthy = Age < 1000 && Health > 0;
    
    //Event<bool>   Migrating = EventSource<bool>();

    SignalT<bool>   ShouldMigrate = Hold(0, FoodReceived) < 10;
    EventsT<bool>   Moving = Pulse(ShouldMigrate, theTime.NewDay);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Run simulation
///////////////////////////////////////////////////////////////////////////////////////////////////
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

template <typename D>
struct Benchmark_LifeSim : public BenchmarkBase<D>
{
    double Run(const BenchmarkParams_LifeSim& params)
    {
        auto theTime    = Time<D>();
        auto theWorld    = World<D>(theTime, params.W);

        auto animals    = vector<unique_ptr<Animal<D>>>();

        std::mt19937 gen(2015);
        std::uniform_int_distribution<int> dist(0,theWorld.Regions.size()-1);

        for (int i=0; i<params.N; i++)
        {
            auto r = theWorld.Regions[dist(gen)].get();
            animals.push_back(unique_ptr<Animal<D>>(new Animal<D>(theTime, theWorld, r, i+1)));
        }

        atomic<int> c(0);
        int s = 0;

        //for (const auto& e : animals)
        //{
        //    Observe(e->Migrating, [&c] (bool _) {
        //        c++;
        //    });
        //}

        //Observe(theEnvironment.SummerIsComing, [] (int v) {
        //    printf("=== SUMMER ===\n");
        //});

        //Observe(theEnvironment.WinterIsComing, [] (int v) {
        //    printf("=== WINTER ===\n");
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

        //s = s / params.K;
        //printf("s %d\n", s);

        s = s / params.K;
        printf("s/k %d\n", s);

/*        std::ofstream logfile;
        logfile.open("log.txt");

        D::Log().Write(logfile);
        logfile.close()*/;

        return d;
    }
};