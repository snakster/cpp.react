
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
/*
#include <iostream>
#include <fstream>
#include <memory>
#include <random>
#include <utility>
#include <string>
#include <type_traits>
#include <tuple>
#include <vector>

#include "BenchmarkBase.h"

#include "react/Domain.h"
#include "react/Signal.h"
#include "react/Event.h"
#include "react/logging/EventLog.h"

using namespace react;

using std::cout;
using std::string;
using std::unique_ptr;
using std::vector;

// FIXME: GCC doesn't like enum class in MakeEventSource
enum    { summer,  winter };
enum    { enter,  leave };

template <typename T>
struct Incrementer { T operator()(Token,T v) const { return v+1; } };

using PositionT = std::pair<int,int>;
using BoundsT = std::tuple<int,int,int,int>;

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Time
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class Time
{
public:
    USING_REACTIVE_DOMAIN(D)

    EventSourceT<>      NewDay      = MakeEventSource<D>();

    SignalT<int>        TotalDays   = Iterate(NewDay, 0, Incrementer<int>());
    SignalT<int>        DayOfYear   = TotalDays % 365;

    SignalT<int> Season = MakeSignal(
        DayOfYear,
        [] (int day) -> int {
            return day < 180 ? winter : summer;
        });
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Region
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class Region
{
    Time<D>& theTime;

public:
    USING_REACTIVE_DOMAIN(D)

    BoundsT Bounds;

    EventSourceT<int>    EnterOrLeave = MakeEventSource<D,int>();

    SignalT<int> AnimalCount = Iterate(
        EnterOrLeave,
        0,
        [] (int m, int count) {
            return m == enter ? count + 1 : count - 1;
        });

    SignalT<int> FoodPerDay = MakeSignal(
        theTime.Season,
        calculateFoodPerDay);

    SignalT<int> FoodOutputPerDay = MakeSignal(
        With(FoodPerDay,AnimalCount),
        calculateFoodOutputPerDay);

    EventsT<int> FoodOutput = Pulse(theTime.NewDay, FoodOutputPerDay);

    Region(Time<D>& time, int x, int y) :
        theTime( time ),
        Bounds( x*10, x*10+9, y*10, y*10+9 )
    {}

    PositionT Center() const
    {
        using std::get;

        return PositionT(get<0>(Bounds) + 5, get<2>(Bounds) + 5);
    }

    PositionT Clamp(PositionT pos) const
    {
        using std::get;

        pos.first  = get<0>(Bounds) + (abs(pos.first) % 10);
        pos.second = get<2>(Bounds) + (abs(pos.second) % 10);

        return  pos;
    }

    bool IsInRegion(PositionT pos) const
    {
        using std::get;

        return  get<0>(Bounds) <= pos.first  && pos.first  <= get<1>(Bounds) &&
                get<2>(Bounds) <= pos.second && pos.second <= get<3>(Bounds);
    }

private:
    static int calculateFoodPerDay(int season)
    {
        return season == summer ? 20 : 10;
    }

    static int calculateFoodOutputPerDay(int food, int count)
    {
        return count > 0 ? food/count : 0;
    }

};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// World
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class World
{
public:
    USING_REACTIVE_DOMAIN(D)

    vector<unique_ptr<Region<D>>>   Regions;

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

        //printf("Out of bounds %d %d\n", pos.first, pos.second);
        assert(false);
        return nullptr;
    }

    PositionT Clamp(PositionT pos) const
    {
        pos.first = abs(pos.first) % 10*w_;
        pos.second = abs(pos.second) % 10*w_;

        return  pos;
    }

private:
    int w_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Animal
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D>
class Animal
{
    Time<D>&    theTime;
    World<D>&   theWorld;

    std::mt19937 generator;

public:
    USING_REACTIVE_DOMAIN(D)

    VarSignalT<Region<D>*>  CurrentRegion;

    EventsT<int>        FoodReceived    = REACTIVE_PTR(CurrentRegion, FoodOutput);
    SignalT<bool>       ShouldMigrate   = Hold(FoodReceived, 0) < 10;
    EventsT<bool>       Moving          = Pulse(theTime.NewDay, ShouldMigrate);

    SignalT<PositionT>  Position;
    SignalT<Region<D>*> NewRegion;

    EventsT<Region<D>*> RegionChanged   = Monitor(NewRegion);

    SignalT<int>        Age             = Iterate(theTime.NewDay, 0, Incrementer<int>());
    SignalT<int>        Health          = Iterate(FoodReceived, 100, calculateHealth);

    Animal(Time<D>& time, World<D>& world, Region<D>* initRegion, unsigned seed) :
        theTime( time ),
        theWorld( world ),
        generator( seed ),
        CurrentRegion( MakeVar<D>(initRegion) ),
        Position
        (
            Iterate(Moving, initRegion->Center(), With(CurrentRegion),
                [this] (bool moving, PositionT position, Region<D>* region) {
                    std::uniform_int_distribution<int> dist(-1,1);

                    // Wander randomly
                    for (int i=0; i<100; i++)
                    {
                        position.first  += dist(generator);
                        position.second += dist(generator);
                    }

                    // Should migrate?
                    if (moving)
                        return theWorld.Clamp(position);
                    else
                        return region->Clamp(position);
                })
        ),
        NewRegion
        (
            MakeSignal(Position, 
                [this] (PositionT pos) {
                    return theWorld.GetRegion(pos);
                })
        ),
        regionChangeContinuation_
        (
            MakeContinuation(RegionChanged, With(CurrentRegion),
                [this] (Region<D>* newRegion, Region<D>* oldRegion) {
                    oldRegion->EnterOrLeave(leave);
                    newRegion->EnterOrLeave(enter);

                    // Change region in continuation
                    CurrentRegion <<= newRegion;
                })
        )
    {
        initRegion->EnterOrLeave(enter);
    }

private:
    Continuation<D> regionChangeContinuation_;

    static int calculateHealth(int food, int health)
    {
        auto newHealth = health + food - 10;
        return newHealth < 0 ? 0 : newHealth > 10000 ? 10000 : newHealth;
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Run simulation
///////////////////////////////////////////////////////////////////////////////////////////////////
struct BenchmarkParams_LifeSim
{
    BenchmarkParams_LifeSim(int n, int w, int k) :
        N( n ), W( w ), K( k )
    {}

    void Print(std::ostream& out) const
    {
        out << "N = " << N
            << ", K = " << K
            << ", W = " << W;
    }

    const int N;
    const int W;
    const int K;
};

template <typename D>
struct Benchmark_LifeSim : public BenchmarkBase<D>
{
    double Run(const BenchmarkParams_LifeSim& params)
    {
        Time<D>  theTime;
        World<D> theWorld( theTime, params.W );

        vector<unique_ptr<Animal<D>>> animals;

        std::mt19937 gen( 2015 );
        std::uniform_int_distribution<size_t> dist( 0u, theWorld.Regions.size()-1 );

        for (int i=0; i<params.N; i++)
        {
            auto r = theWorld.Regions[dist(gen)].get();
            animals.push_back(unique_ptr<Animal<D>>(new Animal<D>(theTime, theWorld, r, i+1)));
        }

        // WHEEL IN THE SKY KEEPS ON TURNING
        auto t0 = tbb::tick_count::now();

        for (int i=0; i<params.K; i++)
            theTime.NewDay();

        auto t1 = tbb::tick_count::now();
        double d = (t1 - t0).seconds();

/*        std::ofstream logfile;
        logfile.open("log.txt");

        D::Log().Write(logfile);
        logfile.close()*//*;

        //return d;
    }
};

*/