## Introduction

Cpp.React is an experimental [Reactive Programming](http://en.wikipedia.org/wiki/Reactive_programming) framework for C++11. Its purpose is to provide abstractions that simplify the implementation of reactive behaviour.

The general idea is that dependency relations between data/actions are expressed declarively, while the actual propagation of changes is handled automatically. The benefits:
* Reduction of boilerplate code.
* Updating is always consistent and glitch-free.
* Support for implicit parallelization of updates.

#### Compiling

Currently, the build has only been tested in Visual Studio 2013.
The Intel C++ Compiler 14.0 with Visual Studio 2012/13 is theoretically supported as well, but it doesn't compile the current codebase anymore due to [some bugs]() with C++11 support.

###### Dependencies
* [Intel TBB 4.2](https://www.threadingbuildingblocks.org/) (required)
* [Google test framework](https://code.google.com/p/googletest/) (optional, to compile the tests)
* [Boost C++ Libraries](http://www.boost.org/) (optional, to use ReactiveLoop, which requires boost::coroutine)

## Features by example

#### Signals

Signals are time-varying reactive values, that can be combined to create reactive expressions.
These expressions are automatically recalculated whenever one of their dependent values changes.

```C++
#include "react/Domain.h"
#include "react/Signal.h"
///...
using namespace react;

REACTIVE_DOMAIN(D);

auto width  = MyDomain::MakeVar(1);
auto height = MyDomain::MakeVar(2);

auto area   = width * height;

cout << "area: " << area() << endl; // => area: 2
width <<= 10;
cout << "area: " << area() << endl; // => area: 20
```

#### Events

Event streams represent flows of discrete values as first-class objects.

```C++
#include "react/Domain.h"
#include "react/Event.h"
//...
using namespace react;

REACTIVE_DOMAIN(MyDomain);

auto leftClicked  = MyDomain::MakeEventSource();
auto rightClicked = MyDomain::MakeEventSource();

auto clicked = leftClicked | rightClicked;

Observe(clicked, [] { cout << "button clicked!" << endl; });
```

#### Implicit parallelism

The change propagation is handled implicitly by a _propagation engine_.
Depending on the selected engine, independent propagation paths are automatically parallelized.

```C++
#include "react/propagation/ToposortEngine.h"
//...
using namespace react;

// Single-threaded updating
REACTIVE_DOMAIN(MyDomain, ToposortEngine<sequential>);

// Parallel updating
REACTIVE_DOMAIN(MyDomain, ToposortEngine<parallel>);

// Input from multiple threads
REACTIVE_DOMAIN(MyDomain, ToposortEngine<sequential_queuing>);
REACTIVE_DOMAIN(MyDomain, ToposortEngine<parallel_queuing>);

// Parallel updating + input from multiple threads + pipelining
REACTIVE_DOMAIN(MyDomain, ToposortEngine<parallel_pipelining>);
```

#### Reactive loops

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
