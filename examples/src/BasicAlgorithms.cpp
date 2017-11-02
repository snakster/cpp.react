
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "react/state.h"
#include "react/event.h"
#include "react/observer.h"
#include "react/algorithm.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 1 - Converting events to signals
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example1
{
    using namespace std;
    using namespace react;

    Group g;

    struct Sensor
    {
        EventSource<int> samples    = EventSource<int>::Create(g);
        State<int>       lastSample = Hold(0, samples);

        auto GetReactiveMembers() const -> decltype(auto)
            { return std::tie(lastSample); }
    };

    void Run()
    {
        cout << "Example 1 - Converting events to signals" << endl;

        Sensor sensor;

        auto obs = Observer::Create(g, [] (int v)
            {
                cout << v << endl;
            }, sensor.lastSample);

        sensor.samples << 20 << 21 << 21 << 22; // output: 20, 21, 22

        g.DoTransaction([&]
            {
                sensor.samples << 30 << 31 << 31 << 32;
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

    Group g;

    struct Employee
    {
        StateVar<string>   name    = StateVar<string>::Create(g, string( "Bob" ));
        StateVar<int>      salary  = StateVar<int>::Create(g, 66666);
    };

    void Run()
    {
        cout << "Example 2 - Converting signals to events" << endl;

        Employee bob;

        auto obs = Observer::Create([] (const auto& events, const string& name)
            {
                for (int newSalary : events)
                    cout << name << " now earns " << newSalary << endl;
            }, Monitor(bob.salary), bob.name);

        bob.salary.Set(66667);

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

    Group g;

    struct Counter
    {
        EventSource<> increment = EventSource<>::Create(g);

        State<int> count = Iterate<int>(0, [] (const auto& events, int count)
            {
                for (auto e : events)
                    ++count;
                return count;
            }, increment);
    };

    void Run()
    {
        cout << "Example 3 - Folding event streams into signals (1)" << endl;

        Counter myCounter;

        myCounter.increment.Emit();
        myCounter.increment.Emit();
        myCounter.increment.Emit();

        auto obs = Observer::Create([] (int v)
            {
                cout << v << endl; // output: 3
            }, myCounter.count);

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

    Group g;

    struct Sensor
    {
        EventSource<float> input = EventSource<float>::Create(g);

        State<int> count = Iterate<int>(0, [] (const auto& events, int count)
            {
                for (auto e : events)
                    ++count;
                return count;
            }, input);

        State<float> sum = Iterate<float>(0.0f, [] (const auto& events, float sum)
            {
                for (auto e : events)
                    sum += e;
                return sum;
            }, input);

        State<float> average = State<float>::Create([] (int c, float s)
            {
                if (c != 0)
                    return s / c;
                else
                    return 0.0f;
                    
            }, count, sum);
    };

    void Run()
    {
        cout << "Example 4 - Folding event streams into signals (2)" << endl;

        Sensor mySensor;

        mySensor.input << 10.0f << 5.0f << 10.0f << 8.0f;

        auto obs = Observer::Create([] (float v)
            {
                cout << "Average: " << v << endl; // output: 8.25
            }, mySensor.average);

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

    Group g;

    enum ECmd { increment, decrement, reset };

    class Counter
    {
    private:
        static int DoCounterLoop(const EventValueList<int>& cmds, int count, int delta, int start)
        {
            for (int cmd : cmds)
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
        EventSource<int> update = EventSource<int>::Create(g);

        StateVar<int> delta = StateVar<int>::Create(g, 1);
        StateVar<int> start = StateVar<int>::Create(g, 0);

        State<int> count = Iterate<int>(0, DoCounterLoop, update, delta, start);
    };

    void Run()
    {
        cout << "Example 5 - Folding event streams into signals (3)" << endl;

        Counter myCounter;

        {
            auto obs = Observer::Create([] (int v)
                {
                    cout << "Start: " << v << endl; // output: 0
                }, myCounter.count);
        }
        

        myCounter.update.Emit(increment);
        myCounter.update.Emit(increment);
        myCounter.update.Emit(increment);

        {
            auto obs = Observer::Create([] (int v)
                {
                    cout << "3x increment by 1: " << v << endl; // output: 3
                }, myCounter.count);
        }

        myCounter.delta.Set(5);
        myCounter.update.Emit(decrement);

        {
            auto obs = Observer::Create([] (int v)
                {
                    cout << "1x decrement by 5: " << v << endl; // output: -2
                }, myCounter.count);
        }        

        myCounter.start.Set(100);
        myCounter.update.Emit(reset);

        {
            auto obs = Observer::Create([] (int v)
                {
                    cout << "reset to 100: " << v << endl; // output: 100
                }, myCounter.count);
        }

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

    Group g;

    class Sensor
    {
    private:
        static void DoIterateAllSamples(const EventValueList<int>& events, vector<int>& all)
        {
            for (int input : events)
                all.push_back(input);
        }

        static void DoIterateCritSamples(const EventValueList<int> events, vector<int>& critical, int threshold)
        {
            for (int input : events)
                if (input > threshold)
                    critical.push_back(input);
        }

    public:
        EventSource<int> input = EventSource<int>::Create(g);

        StateVar<int> threshold = StateVar<int>::Create(g, 42);

        State<vector<int>> allSamples = IterateByRef<vector<int>>(vector<int>{ }, DoIterateAllSamples, input);

        State<vector<int>> criticalSamples = IterateByRef<vector<int>>(vector<int>{ }, DoIterateCritSamples, input, threshold);
    };

    void Run()
    {
        cout << "Example 6 - Avoiding expensive copies" << endl;

        Sensor mySensor;

        mySensor.input << 40 << 29 << 43 << 50;

        cout << "All samples: ";
        {
            auto obs = Observer::Create([] (const auto& allSamples)
                {
                    for (auto const& v : allSamples)
                        cout << v << " ";
                }, mySensor.allSamples);
        }
        cout << endl;

        cout << "Critical samples: ";
        {
            auto obs = Observer::Create([] (const auto& criticalSamples)
                {
                    for (auto const& v : criticalSamples)
                        cout << v << " ";
                }, mySensor.criticalSamples);
        }
        cout << endl;
    }
}


using namespace react;

Group g;

struct MyClass
{
    StateVar<int> a = StateVar<int>::Create(g, 10);
    StateVar<int> b = StateVar<int>::Create(g, 20);

    bool operator==(const MyClass& other)
        { return a == other.a; }

    struct Flat;
};

struct MyClass::Flat : public Flattened<MyClass>
{
    using Flattened::Flattened;

    Ref<int> a = this->Flatten(MyClass::a);
    Ref<int> b = this->Flatten(MyClass::b);
};

using namespace react;
using namespace std;

void test1()
{
    

    /*StateVar<int> x;
    State<int> y;
    State<int> z;

    State<std::vector<State<int>>>   list;
    State<std::map<int, State<int>>> map;

    State<std::vector<int>> flatlist = FlattenList(list);
    State<std::map<int, int>> flatmap = FlattenMap(map);

    MyClass cls1;
    MyClass::Flat cls2(cls1);

    StateVar<State<int>> sig;

    Flatten(g, sig);*/

    auto w1 = StateVar<string>::Create(g, "Widget1");
    auto w2 = StateVar<string>::Create(g, "Widget2");
    auto w3 = StateVar<string>::Create(g, "Widget3");

    auto allWidgets = { w1, w2, w3 };

    auto d1 = StateVar<string>::Create(g, "Data1");
    auto d2 = StateVar<string>::Create(g, "Data2");
    auto d3 = StateVar<string>::Create(g, "Data3");

    auto allData = { d1, d2, d3 };

    auto objects = StateVar<vector<StateVar<string>>>::Create(g);

    auto obs = Observer::Create([] (const auto& flatList)
        {
            cout << "Objects: ";
            for (const string& s : flatList)
                cout << s << " ";
            cout << endl;
        }, FlattenList(objects));
    // Objects:

    objects.Modify([&] (auto& w)
        {
            w.push_back(w1);
        });
    // Objects: Widget1

    w1.Set("Widget1 (x)");
    // Objects: Widget1 (x)

    objects.Set(allWidgets);
    // Objects: Widget1 (x) Widget2 Widget3

    objects.Set(allData);
    // Objects: Data1 Data2 Data3

    w2.Set("Widget2 (x)");
    
    g.DoTransaction([&]
        {
            w3.Set("Widget3 (x)");

            d1.Set("Data1 (x)");
            d2.Set("Data2 (x)");

            objects.Set(allWidgets);
        });
    // Objects: Widget1 (x) Widget2 (x) Widget3 (x)

    objects.Modify([&] (auto& w)
        {
            w.clear();
            w.insert(end(w), allWidgets);
            w.insert(end(w), allData);
        });
    // Objects: Widget1 (x) Widget2 (x) Widget3 (x) Data1 (x) Data2 (x) Data3
    
}

class Office;
class Company;
class Employee;

//----
class Office
{
public:
    StateVar<string> location;

    StateVar<vector<State<Employee>>> employees;

    State<int> employeeCount = State<int>::Create(CalcEmployeeCount, FlattenList(employees));

    struct Flat;

private:
    static int CalcEmployeeCount(const vector<Employee>& offices)
        { return offices.size(); }
};

struct Office::Flat : public Flattened<Office>
{
    using Flattened::Flattened;

    int employeeCount = this->Flatten(Office::employeeCount);
};

//----
class Employee
{
public:
    StateVar<string> name;

    StateRef<Office> office;

    struct Flat;

private:
};

struct Employee::Flat : public Flattened<Employee>
{
    using Flattened::Flattened;

    Ref<string> name = this->Flatten(Employee::name);
    Ref<Office> office = this->Flatten(Employee::office);
};

//----
class Company
{
public:
    StateVar<vector<State<Office>>> offices;

    State<int> employeeCount;

    struct Flat;

private:
    static int CalcEmployeeCount(const vector<Office::Flat>& offices)
    {
        int count = 0;

        for (const auto& office : offices)
            count += office.employeeCount;

        return count;
    }
};

struct Company::Flat : public Flattened<Company>
{
    using Flattened::Flattened;

    Ref<vector<State<Office>>> offices = this->Flatten(Company::offices);
    int employeeCount = this->Flatten(Company::employeeCount);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Run examples
///////////////////////////////////////////////////////////////////////////////////////////////////
int main()
{
    /*example1::Run();
    example2::Run();
    example3::Run();
    example4::Run();
    example5::Run();
    example6::Run();*/

    test1();

    return 0;
}