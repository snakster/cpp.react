
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "react/Domain.h"
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

    REACTIVE_DOMAIN(D, sequential)

    class Sensor
    {
    public:
        USING_REACTIVE_DOMAIN(D)

        EventSourceT<int>   Samples     = MakeEventSource<D,int>();
        SignalT<int>        LastSample  = Hold(Samples, 0);
    };

    void Run()
    {
        cout << "Example 1 - Converting events to signals" << endl;

        Sensor mySensor;

        Observe(mySensor.LastSample, [] (int v) {
            std::cout << v << std::endl;
        });

        mySensor.Samples << 20 << 21 << 21 << 22; // output: 20, 21, 22

        DoTransaction<D>([&] {
            mySensor.Samples << 30 << 31 << 31 << 32;
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

    REACTIVE_DOMAIN(D, sequential)

    class Employee
    {
    public:
        USING_REACTIVE_DOMAIN(D)

        VarSignalT<string>   Name    = MakeVar<D>(string( "Bob" ));
        VarSignalT<int>      Salary  = MakeVar<D>(3000);

        EventsT<int>         SalaryChanged = Monitor(Salary);
    };

    void Run()
    {
        cout << "Example 2 - Converting signals to events" << endl;

        Employee bob;

        Observe(
            bob.SalaryChanged,
            With(bob.Name),
            [] (int newSalary, const string& name) {
                cout << name << " now earns " << newSalary << endl;
            });

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

    REACTIVE_DOMAIN(D, sequential)

    class Counter
    {
    public:
        USING_REACTIVE_DOMAIN(D)

        EventSourceT<>  Increment = MakeEventSource<D>();

        SignalT<int> Count = Iterate(
            Increment,
            0,
            [] (Token, int oldCount) {
                return oldCount + 1;
            });
    };

    void Run()
    {
        cout << "Example 3 - Folding event streams into signals (1)" << endl;

        Counter myCounter;

        // Note: Using function-style operator() instead of .Emit() and .Value()
        myCounter.Increment();
        myCounter.Increment();
        myCounter.Increment();

        cout << myCounter.Count() << endl; // output: 3

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

    REACTIVE_DOMAIN(D, sequential)

    class Sensor
    {
    public:
        USING_REACTIVE_DOMAIN(D)

        EventSourceT<float> Input = MakeEventSource<D,float>();

        SignalT<int> Count = Iterate(
            Tokenize(Input),
            0,
            [] (Token, int oldCount) {
                return oldCount + 1;
            });

        SignalT<float> Sum = Iterate(
            Input,
            0.0f,
            [] (float v, float sum) {
                return v + sum;
            });

        SignalT<float> Average = MakeSignal(
            With(Sum,Count),
            [] (float sum, int count) {
                return count != 0 ? sum / count : 0.0f;
            });
    };

    void Run()
    {
        cout << "Example 4 - Folding event streams into signals (2)" << endl;

        Sensor mySensor;

        mySensor.Input << 10.0f << 5.0f << 10.0f << 8.0f;

        cout << "Average: " << mySensor.Average() << endl; // output: 8.25

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

    REACTIVE_DOMAIN(D, sequential)

    enum ECmd { increment, decrement, reset };

    class Counter
    {
    public:
        USING_REACTIVE_DOMAIN(D)

        EventSourceT<int>   Update  = MakeEventSource<D,int>();
        VarSignalT<int>     Delta   = MakeVar<D>(1);
        VarSignalT<int>     Start   = MakeVar<D>(0);

        SignalT<int> Count = Iterate(
            Update,
            Start.Value(),
            With(Delta, Start),
            [] (int cmd, int oldCount, int delta, int start) {
                if (cmd == increment)
                    return oldCount + delta;
                else if (cmd == decrement)
                    return oldCount - delta;
                else
                    return start;
            });
    };

    void Run()
    {
        cout << "Example 5 - Folding event streams into signals (3)" << endl;

        Counter myCounter;

        cout << "Start: " << myCounter.Count() << endl; // output: 0

        myCounter.Update(increment);
        myCounter.Update(increment);
        myCounter.Update(increment);

        cout << "3x increment by 1: " << myCounter.Count() << endl; // output: 3

        myCounter.Delta <<= 5;
        myCounter.Update(decrement);

        cout << "1x decrement by 5: " << myCounter.Count() << endl; // output: -2

        myCounter.Start <<= 100;
        myCounter.Update(reset);

        cout << "reset to 100: " << myCounter.Count() << endl; // output: 100

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

    REACTIVE_DOMAIN(D, sequential)

    class Sensor
    {
    public:
        USING_REACTIVE_DOMAIN(D)

        EventSourceT<int>   Input = MakeEventSource<D,int>();

        VarSignalT<int>     Threshold = MakeVar<D>(42);

        SignalT<vector<int>> AllSamples = Iterate(
            Input,
            vector<int>{},
            [] (int input, vector<int>& all) {
                all.push_back(input);
            });

        SignalT<vector<int>> CriticalSamples = Iterate(
            Input,
            vector<int>{},
            With(Threshold),
            [] (int input, vector<int>& critical, int threshold) {
                if (input > threshold)
                    critical.push_back(input);
            });
    };

    void Run()
    {
        cout << "Example 6 - Avoiding expensive copies" << endl;

        Sensor mySensor;

        mySensor.Input << 40 << 29 << 43 << 50;

        cout << "All samples: ";
        for (auto const& v : mySensor.AllSamples())
            cout << v << " ";
        cout << endl;

        cout << "Critical samples: ";
        for (auto const& v : mySensor.CriticalSamples())
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