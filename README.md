## Introduction

Cpp.React is an experimental [Reactive Programming](http://en.wikipedia.org/wiki/Reactive_programming) framework for C++11.
It provides abstractions to simplify the implementation of reactive behaviour.
This is accomplished by enabling the declarative expression of dataflows and handling the propagation of changes automatically.
Implicit parallelism for this process is supported as well.

#### Building

So far, I've only tested building the framework on Windows, with:
* Visual Studio 2013
* Intel C++ Compiler 14.0 in Visual Studio 2012/13

###### Dependencies
* [Intel TBB 4.2](https://www.threadingbuildingblocks.org/) (required)
* [Google test framework](https://code.google.com/p/googletest/) (optional, to compile the tests)
* [Boost C++ Libraries](http://www.boost.org/) (optional, to use ReactiveLoop, which requires boost::coroutine)

Cpp.React uses standard C++11 and the dependencies are portable, so other compilers/platforms should work, too.


## Feature overview

#### Signals

Signals are time-varying reactive values, that can be combined to create reactive expressions.
These expressions are automatically recalculated whenever one of their dependent values changes.
Example:

```C++
#include "react/Signal.h"

using namespace react;

REACTIVE_DOMAIN(MyDomain);

auto width  = MyDomain::MakeVar(1);
auto height = MyDomain::MakeVar(2);

auto area   = width * height;

cout << "area: "   << area()   << endl; // => area: 2
width <<= 10;
cout << "area: "   << area()   << endl; // => area: 20
```

#### Event streams

Event streams represent flows of discrete values as first-class objects, based on ideas found in [Deprecating the Observer Pattern](http://infoscience.epfl.ch/record/176887/files/DeprecatingObservers2012.pdf). Example:

```C++
#include "react/EventStream.h"

using namespace react;

REACTIVE_DOMAIN(MyDomain);

auto leftClicked  = MyDomain::MakeEventSource();
auto rightClicked = MyDomain::MakeEventSource();

auto clicked = leftClicked | rightClicked;

Observe(clicked, [] { cout << "button clicked!" << endl; });
```

#### Implicit parallelism

The change propagation is handled implicitly by a so called propagation engine.
Depending on the selected engine, independent propagation paths are automatically parallelized.
For more details, see Propagation Engines.

```C++
#include "react/propagation/TopoSortEngine.h"

...

// Single-threaded updating
REACTIVE_DOMAIN(MyDomain, TopoSortEngine<sequential>);

// Parallel updating
REACTIVE_DOMAIN(MyDomain, TopoSortEngine<parallel>);

// Input from multiple threads
REACTIVE_DOMAIN(MyDomain, TopoSortEngine<sequential_queuing>);
REACTIVE_DOMAIN(MyDomain, TopoSortEngine<parallel_queuing>);

// Parallel updating + input from multiple threads + pipelining
REACTIVE_DOMAIN(MyDomain, TopoSortEngine<parallel_pipelining>);
```

#### Reactive loops

```C++
#include "react/Reactor.h"

REACTIVE_DOMAIN(D);

cout << "ReactiveLoop Example 1" << endl;

using PointT = pair<int,int>;
using PathT  = vector<PointT>;

vector<PathT> paths;

auto mouseDown = D::MakeEventSource<PointT>();
auto mouseUp   = D::MakeEventSource<PointT>();
auto mouseMove = D::MakeEventSource<PointT>();

D::ReactiveLoop loop
{
	[&] (D::ReactiveLoop::Context& ctx)
	{
		PathT points;

		points.emplace_back(ctx.Take(mouseDown));

		ctx.RepeatUntil(mouseUp, [&] {
			points.emplace_back(ctx.Take(mouseMove));
		});

		points.emplace_back(ctx.Take(mouseUp));

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

for (const auto& path : paths)
{
	cout << "Path: ";
	for (const auto& point : path)
		cout << "(" << point.first << "," << point.second << ")   ";
	cout << endl;
}
```

### More examples

* [Examples](https://github.com/schlangster/cpp.react/blob/master/src/sandbox/Main.cpp)
* [Benchmark](https://github.com/schlangster/cpp.react/blob/master/src/benchmark/BenchmarkLifeSim.h)
* [Test cases](https://github.com/schlangster/cpp.react/tree/master/src/test)
