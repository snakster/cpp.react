
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdio>
#include <vector>
#include <chrono>
#include <thread>
#include <tbb/tick_count.h>

#include "react/Signal.h"
#include "react/Event.h"
#include "react/Algorithm.h"
#include "react/Observer.h"

using namespace react;


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

    void Generate(const ReactiveGroupBase& group)
    {
        assert(inputSignals.size() >= 1);
        assert(widths.size() >= 1);

        SignalVectType buf1 = std::move(inputSignals);
        SignalVectType buf2;

        SignalVectType* curBuf = &buf1;
        SignalVectType* nextBuf = &buf2;

        size_t curWidth = buf1.size();

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


template <typename T>
T Multiply(T a, T b)
{
	return a * b;
}

template <typename T> void PrintValue(T v)
{
	printf("Value: %d\n", v);
}

template <typename T> void PrintArea(T v)
{
	printf("Area: %d\n", v);
}

template <typename T> void PrintVolume(T v)
{
	printf("Volume: %d\n", v);
}

template <typename T> void PrintEvents(EventRange<T> evts)
{
	printf("Processing events...\n");

	for (const auto& e : evts)
		printf("  Event: %d\n", e);
}

template <typename T> void PrintSyncedEvents(EventRange<T> evts, int a, int b)
{
	printf("Processing events...\n");

	for (const auto& e : evts)
		printf("  Event: %d, %d, %d\n", e, a, b);
}

template <typename T> bool FilterFunc(T v)
{
	return v > 10;
}

template <typename T> T IterFunc1(EventRange<T> evts, T v)
{
	return v + 1;
}

template <typename T> T IterFunc2(EventRange<T> evts, T v, T a1, T a2)
{
	return v + 1;
}

int main()
{
	ReactiveGroup<> group;

	{
		// Signals
		VarSignal<int> x{ group, 0 };
		VarSignal<int> y{ group, 0 };
		VarSignal<int> z{ group, 0 };

		Signal<int> area{ Multiply<int>, x, y };
		Signal<int> volume{ Multiply<int>, area, z };

		Observer<> areaObs{ PrintArea<int>, area };
		Observer<> volumeObs{ PrintVolume<int>, volume };

		x.Set(2); // a: 0, v: 0
		y.Set(2); // a: 4, v: 0
		z.Set(2); // a: 4, v: 8

		group.DoTransaction([&]
		{
			x.Set(100);
			y <<= 3;
			y <<= 4;
		});

		// a: 400, v: 800
	}

	{
		// Events
		EventSource<int> button1{ group };
		EventSource<int> button2{ group };

		Event<int> anyButton = Merge(button1, button2);
		Event<int> filtered  = Filter(FilterFunc<int>, anyButton);

		Observer<> eventObs{ PrintEvents<int>, anyButton };

		button1.Emit(1);
		button2.Emit(2);

		group.DoTransaction([&]
		{
			for (int i=0; i<10; ++i)
				button1.Emit(42);
		});
	}

	{
		// Dynamic signals
		VarSignal<int> s1{ group, 10 };
		VarSignal<int> s2{ group, 22 };

		SignalSlot<int> slot{ s1 };

		Observer<> areaObs{ PrintValue<int>, slot };

		s1.Set(42);

		slot.Set(s2);

		s2.Set(667);
	}

	// Links
	{
		ReactiveGroup<> group1;
		ReactiveGroup<> group2;

		VarSignal<int> s1{ group1, 10 };
		VarSignal<int> s2{ group2, 11 };

		Signal<int> v{ Multiply<int>, s1, s2 };

		Observer<> obs{ PrintValue<int>, v };

		s1.Set(555);

		std::this_thread::sleep_for(std::chrono::seconds(5));
	}

	{
		ReactiveGroup<> group1;
		ReactiveGroup<> group2;

		VarSignal<int> s1{ group1, 10 };
		VarSignal<int> s2{ group2, 11 };

		EventSource<int> e1{ group1 };
		EventSource<int> e2{ group2 };

		auto hold = Hold(group1, 0, e1);

		auto merged = Merge(group2, e1, e2);

		auto joined1 = Join(e1, e2);
		auto joined2 = Join(group1, e1, e2);

		Observer<> eventObs1{ PrintEvents<int>, merged };
		Observer<> eventObs2{ group2, PrintSyncedEvents<int>, merged, s1, s2 };

		e1.Emit(222);

		std::this_thread::sleep_for(std::chrono::seconds(5));
	}

	{
		ReactiveGroup<> group1;
		ReactiveGroup<> group2;

		VarSignal<int> s1{ group1, 10 };
		VarSignal<int> s2{ group2, 11 };

		EventSource<int> e1{ group1 };
		EventSource<int> e2{ group2 };

		auto hold1 = Hold(group1, 0, e1);
		auto hold2 = Hold(0, e1);

		auto monitor1 = Monitor(group1, s1);
		auto monitor2 = Monitor(s1);

		auto snapshot1 = Snapshot(group1, s1, e1);
		auto snapshot2 = Snapshot(s1, e1);

		auto pulse1 = Pulse(group1, s1, e1);
		auto pulse2 = Pulse(s1, e1);

		auto merged = Merge(group2, e1, e2);

		auto joined1 = Join(e1, e2);
		auto joined2 = Join(group1, e1, e2);

		auto iter1 = Iterate<int>(group, 0, IterFunc1<int>, e1);
		auto iter2 = Iterate<int>(0, IterFunc1<int>, e1);

		auto iter3 = Iterate<int>(group, 0, IterFunc2<int>, e1, s1, s2);
		auto iter4 = Iterate<int>(0, IterFunc2<int>, e1, s1, s2);

		Observer<> eventObs{ PrintEvents<int>, merged };
		Observer<> eventObs2{ group2, PrintSyncedEvents<int>, merged, s1, s2 };

		e1.Emit(222);

		std::this_thread::sleep_for(std::chrono::seconds(5));

		GridGraphGenerator<int> grid;
	}

	return 0;
}











int main()
{
	ReactiveGroup<> group;

	VarSignal<int, shared> in{ group, 1 };
	Signal<int, shared> in2 = in;

	GridGraphGenerator<int> generator;

	generator.inputSignals.push_back(in2);

	generator.widths.push_back(100);
	generator.widths.push_back(1);

	int updateCount = 0;

	generator.function1 = [&] (int a) { ++updateCount; return a; };
	generator.function2 = [&] (int a, int b) { ++updateCount; return a + b; };

	generator.Generate(group);

	updateCount = 0;

	auto t0 = tbb::tick_count::now();
	for (int i = 0; i < 10000; i++)
		in <<= 10 + i;
	auto t1 = tbb::tick_count::now();

	double d = (t1 - t0).seconds();
	printf("updateCount %d\n", updateCount);
	printf("Time %g\n", d);

	return 0;
}
















/*int main2()
{
	ReactiveGroup<> group1;
	ReactiveGroup<> group2;
	ReactiveGroup<> group3;

	VarSignal<int> x{ 0, group1 };
	VarSignal<int> y{ 0, group2 };
	VarSignal<int> z{ 0, group3 };

	Signal<int> area{ Multiply<int>, x, y };
	Signal<int> volume{ Multiply<int>, area, z };

	Observer<> obs{ PrintAreaAndVolume, area, volume };

	Signal<vector<int>> volumeHistory = Iterate<vector<int>>( vector<int>{ }, PushToVector, Monitor(volume));

	x <<= 2;
	y <<= 2;
	z <<= 2;

	group.DoTransaction([&]
	{
		x <<= 100;
		y <<= 200;
		z <<= 300;
	});

	obs.Cancel();

	x <<= 42;

	printf("History:\n");
	for (auto t : volumeHistory.Value())
		printf("%d ", t);
	printf("\n");

	return 0;
}


int main3()
{
    using namespace std;
    using namespace react;

    ReactiveGroup<> group1;
		ReactiveGroup<> group2;

    auto sig1 = VarSignal<int>( 10, group1 );

    auto link1 = SignalLink<int>( sig1, group2 );
		auto link2 = SignalLink<int>( sig1, group2 );

		sig1.Set(10);

    return 0;
}*/