
//          Copyright Sebastian Jeckel 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "react/Signal.h"
#include "react/Event.h"
#include "react/Observer.h"
#include "react/Algorithm.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 1 - Converting events to signals
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example1
{
    using namespace std;
    using namespace react;

    ReactiveGroup group;

    struct Sensor
    {
        EventSource<int>   samples      { group };
        Signal<int>        lastSample   = Hold(0, samples);
    };

    void Run()
    {
        cout << "Example 1 - Converting events to signals" << endl;

        Sensor mySensor;

        Observer obs(
            [] (int v)
            {
                cout << v << endl;
            },
            mySensor.lastSample);

        mySensor.samples << 20 << 21 << 21 << 22; // output: 20, 21, 22

        group.DoTransaction([&]
        {
            mySensor.samples << 30 << 31 << 31 << 32;
        }); // output: 32

        cout << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 2 - Converting signals to events
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example2
{
    using namespace std;
    using namespace react;

    ReactiveGroup group;

    struct Employee
    {
        VarSignal<string>   name    { group, string( "Bob" ) };
        VarSignal<int>      salary  { group, 3000 };
    };

    void Run()
    {
        cout << "Example 2 - Converting signals to events" << endl;

        Employee bob;

        Observer obs(
            [] (EventRange<int> in, const string& name)
            {
                for (int newSalary : in)
                    cout << name << " now earns " << newSalary << endl;
            },
            Monitor(bob.salary), bob.name);

        cout << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 3 - Folding event streams into signals (1)
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example3
{
    using namespace std;
    using namespace react;

    ReactiveGroup group;

    struct Counter
    {
        EventSource<> increment { group };

        Signal<int> count = Iterate<int>(
            0,
            [] (EventRange<> in, int count)
            {
                for (auto _ : in)
                    ++count;
                return count;
            },
            increment);
    };

    void Run()
    {
        cout << "Example 3 - Folding event streams into signals (1)" << endl;

        Counter myCounter;

        myCounter.increment.Emit();
        myCounter.increment.Emit();
        myCounter.increment.Emit();

        cout << myCounter.count.Value() << endl; // output: 3

        cout << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 4 - Folding event streams into signals (2)
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example4
{
    using namespace std;
    using namespace react;

    ReactiveGroup group;

    struct Sensor
    {
        EventSource<float> input{ group };

        Signal<int> count = Iterate<int>(
            0,
            [] (EventRange<float> in, int count)
            {
                for (auto _ : in)
                    count++;
                return count;
            },
            input);

        Signal<float> sum = Iterate<float>(
            0.0f,
            [] (EventRange<float> in, float sum)
            {
                for (auto v : in)
                    sum += v;
                return sum;
            },
            input);

        VarSignal<float> average{ group };
    };

    void Run()
    {
        cout << "Example 4 - Folding event streams into signals (2)" << endl;

        Sensor mySensor;

        mySensor.input << 10.0f << 5.0f << 10.0f << 8.0f;

        cout << "Average: " << mySensor.average.Value() << endl; // output: 8.25

        cout << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 5 - Folding event streams into signals (3)
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example5
{
    using namespace std;
    using namespace react;

    ReactiveGroup group;

    enum ECmd { increment, decrement, reset };

    class Counter
    {
    private:
        static int DoCounterLoop(EventRange<int> in, int count, int delta, int start)
        {
            for (int cmd : in)
            {
                if (cmd == increment)
                    count += delta;
                else if (cmd == decrement)
                    count -= delta;
                else
                    count = start;
            }

            return count;
        }

    public:
        EventSource<int> update{ group };

        VarSignal<int> delta{ group, 1 };
        VarSignal<int> start{ group, 0 };

        Signal<int> count{ Iterate<int>(start.Value(), DoCounterLoop, update, delta, start) };
    };

    void Run()
    {
        cout << "Example 5 - Folding event streams into signals (3)" << endl;

        Counter myCounter;

        cout << "Start: " << myCounter.count.Value() << endl; // output: 0

        myCounter.update.Emit(increment);
        myCounter.update.Emit(increment);
        myCounter.update.Emit(increment);

        cout << "3x increment by 1: " << myCounter.count.Value() << endl; // output: 3

        myCounter.delta <<= 5;
        myCounter.update.Emit(decrement);

        cout << "1x decrement by 5: " << myCounter.count.Value() << endl; // output: -2

        myCounter.start <<= 100;
        myCounter.update.Emit(reset);

        cout << "reset to 100: " << myCounter.count.Value() << endl; // output: 100

        cout << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 6 - Avoiding expensive copies
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example6
{
    using namespace std;
    using namespace react;

    ReactiveGroup group;

    class Sensor
    {
    private:
        static void DoIterateAllSamples(EventRange<int> in, vector<int>& all)
        {
            for (int input : in)
                all.push_back(input);
        }

        static void DoIterateCritSamples(EventRange<int> in, vector<int>& critical, int threshold)
        {
            for (int input : in)
                if (input > threshold)
                    critical.push_back(input);
        }

    public:
        EventSource<int> input{ group };

        VarSignal<int> threshold{ group, 42 };

        Signal<vector<int>> allSamples{ Iterate<vector<int>>(vector<int>{ }, DoIterateAllSamples, input) };

        Signal<vector<int>> criticalSamples{ Iterate<vector<int>>(vector<int>{ }, DoIterateCritSamples, input, threshold) };
    };

    void Run()
    {
        cout << "Example 6 - Avoiding expensive copies" << endl;

        Sensor mySensor;

        mySensor.input << 40 << 29 << 43 << 50;

        cout << "All samples: ";
        for (auto const& v : mySensor.allSamples.Value())
            cout << v << " ";
        cout << endl;

        cout << "Critical samples: ";
        for (auto const& v : mySensor.criticalSamples.Value())
            cout << v << " ";
        cout << endl;

        cout << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Run examples
///////////////////////////////////////////////////////////////////////////////////////////////////
int main()
{
    example1::Run();
    example2::Run();
    example3::Run();
    example4::Run();
    example5::Run();
    example6::Run();

    return 0;
}