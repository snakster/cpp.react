
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdio>
#include <vector>
#include <chrono>
#include <thread>

#include "react/Signal.h"
#include "react/Event.h"
#include "react/Algorithm.h"
#include "react/Observer.h"

using namespace react;

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

int main()
{
	ReactiveGroup<> group;

	{
		// Signals
		VarSignal<int> x{ group, 0 };
		VarSignal<int> y{ group, 0 };
		VarSignal<int> z{ group, 0 };

		Signal<int> area{ group, Multiply<int>, x, y };
		Signal<int> volume{ Multiply<int>, area, z };

		Observer<> areaObs{ PrintArea<int>, area };
		Observer<> volumeObs{ PrintVolume<int>, volume };

		x.Set(2); // a: 0, v: 0
		y.Set(2); // a: 4, v: 0
		z.Set(2); // a: 4, v: 8

		group.DoTransaction([&]
		{
			x <<= 100;
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

		auto joined = Join(e1, e2);
		auto joined2 = Join(group1, e1, e2);

		Observer<> eventObs{ PrintEvents<int>, merged };
        Observer<> eventObs2{ group2, PrintSyncedEvents<int>, merged, s1, s2 };

		e1.Emit(222);

		std::this_thread::sleep_for(std::chrono::seconds(5));
	}

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