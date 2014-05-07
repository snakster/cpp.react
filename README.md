## Introduction

Cpp.React is an experimental [Reactive Programming](http://en.wikipedia.org/wiki/Reactive_programming) framework for C++11.

It provides abstractions to simplify the implementation of reactive behaviour required by modern applications.

As an alternative to callbacks, it offers the following benefits:
* Less boilerplate code;
* consistent updating without redundant calculations or glitches;
* implicit parallelization.

#### Compiling

So far, the build has only been tested in Visual Studio 2013 as it's the development environment I'm using.
The Intel C++ Compiler 14.0 with Visual Studio 2012/13 is theoretically supported as well, but last I checked, it did not compile anymore due to [some bugs]() with C++11 support.

You are encouraged to try compiling it with other C++11 compilers and tell me why it didn't work :)

###### Dependencies
* [Intel TBB 4.2](https://www.threadingbuildingblocks.org/) (required)
* [Google test framework](https://code.google.com/p/googletest/) (optional, to compile the tests)
* [Boost C++ Libraries](http://www.boost.org/) (optional, to use ReactiveLoop, which requires boost::coroutine)

## Features by example

#### Signals

Signals are time-varying reactive values.
They can be combined to expressions to create new signals, which are automatically recalculated whenever one of their data dependencies changes.
As such, they can be seen as self-updating variables.

```C++
#include "react/Domain.h"
#include "react/Signal.h"
///...
using namespace react;

REACTIVE_DOMAIN(D);

auto width  = D::MakeVar(1);
auto height = D::MakeVar(2);

auto area   = width * height;

cout << "area: " << area() << endl; // => area: 2
width <<= 10;
cout << "area: " << area() << endl; // => area: 20
```

#### Events

Event streams represent flows of discrete values.
They are first-class objects, so they can be merged, filtered, transformed or composed to more complex types.

```C++
#include "react/Domain.h"
#include "react/Event.h"
//...
using namespace react;

REACTIVE_DOMAIN(D);

auto leftClicked  = D::MakeEventSource();
auto rightClicked = D::MakeEventSource();

auto clicked = leftClicked | rightClicked;

Observe(clicked, [] { cout << "button clicked!" << endl; });
```

#### Implicit parallelism

The change propagation is handled implicitly by a _propagation engine_.
Depending on the selected engine, independent propagation paths are automatically parallelized.

```C++
#include "react/Domain.h"
#include "react/Signal.h"
#include "react/engine/ToposortEngine.h"

using namespace react;

int main()
{
    // Sequential
    {
        REACTIVE_DOMAIN(D, ToposortEngine<sequential>);

        auto a = D::MakeVar(1);
        auto b = D::MakeVar(2);
        auto c = D::MakeVar(3);

        auto x = (a + b) * c;

        b <<= 20;
    }

    // Parallel
    {
        REACTIVE_DOMAIN(D, ToposortEngine<parallel>);

        auto in = D::MakeVar(0);

        auto op1 = in ->* [] (int in)
        {
            int result = in /* Costly operation #1 */;
            return result;
        };

        auto op2 = in ->* [] (int in)
        {
            int result = in /* Costly operation #2 */;
            return result;
        };

        auto out = op1 + op2;

        // op1 and op2 can be re-calculated in parallel
        in <<= 123456789;
    }

    // Queued input
    {
        REACTIVE_DOMAIN(D, ToposortEngine<sequential_queue>);

        auto a = D::MakeVar(1);
        auto b = D::MakeVar(2);
        auto c = D::MakeVar(3);

        auto x = (a + b) * c;

        // Thread-safe input from multiple-threads (Note: unspecified order)
        std::thread t1{ [&] { a <<= 10; } };
        std::thread t2{ [&] { b <<= 100; } };
        std::thread t3{ [&] { a <<= 1000; } };
        std::thread t4{ [&] { b <<= 10000; } };

        t1.join(); t2.join(); t3.join(); t4.join();
    }
}
```

#### Reactive loops
(Note: Experimental)

```C++
#include "react/Domain.h"
#include "react/Event.h"
#include "react/Reactor.h"
//...
using namespace std;
using namespace react;

REACTIVE_DOMAIN(D);

using PointT = pair<int,int>;
using PathT  = vector<PointT>;

vector<PathT> paths;

auto mouseDown = D::MakeEventSource<PointT>();
auto mouseUp   = D::MakeEventSource<PointT>();
auto mouseMove = D::MakeEventSource<PointT>();

D::ReactiveLoopT loop
{
	[&] (D::ReactiveLoopT::Context& ctx)
	{
		PathT points;

		points.emplace_back(ctx.Await(mouseDown));

		ctx.RepeatUntil(mouseUp, [&] {
			points.emplace_back(ctx.Await(mouseMove));
		});

		points.emplace_back(ctx.Await(mouseUp));

		paths.push_back(points);
	}
};

mouseDown << PointT(1,1);
mouseMove << PointT(2,2) << PointT(3,3) << PointT(4,4);
mouseUp   << PointT(5,5);

mouseMove << PointT(999,999);

mouseDown << PointT(10,10);
mouseMove << PointT(20,20);
mouseUp   << PointT(30,30);

// => paths[0]: (1,1) (2,2) (3,3) (4,4) (5,5)
// => paths[1]: (10,10) (20,20) (30,30)
```

#### Reactive objects and dynamic reactives

```C++
#include "react/Domain.h"
#include "react/Signal.h"
#include "react/ReactiveObject.h"
//...
using namespace std;
using namespace react;

REACTIVE_DOMAIN(D);

class Company : public ReactiveObject<D>
{
public:
    VarSignalT<string>    Name;

    Company(const char* name) :
        Name{ MakeVar(string(name)) }
    {}

    inline bool operator==(const Company& other) const { /* ... */ }
};

class Manager : public ReactiveObject<D>
{
    ObserverT nameObs;

public:
    VarSignalT<Company&>    CurrentCompany;

    Manager(initialCompany& company) :
        CurrentCompany{ MakeVar(ref(company)) }
    {
        nameObs = REACTIVE_REF(CurrentCompany, Name).Observe([] (string name) {
            cout << "Manager: Now managing " << name << endl;
        });
    }
};

Company company1{ "Cellnet" };
Company company2{ "Borland" };

Manager manager{ company1 };

company1.Name <<= string("BT Cellnet");
company2.Name <<= string("Inprise");

manager.CurrentCompany <<= ref(company2);

company1.Name <<= string("O2");
company2.Name <<= string("Borland");
```

### More examples

* [Examples](https://github.com/schlangster/cpp.react/blob/master/src/sandbox/Main.cpp)
* [Benchmark](https://github.com/schlangster/cpp.react/blob/master/src/benchmark/BenchmarkLifeSim.h)
* [Test cases](https://github.com/schlangster/cpp.react/tree/master/src/test)
