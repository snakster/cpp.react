### Introduction

Cpp.React is an experimental [Reactive Programming](http://en.wikipedia.org/wiki/Reactive_programming) framework for C++11.
It provides abstractions to simplify the implementation of reactive behaviour.
This is accomplished by enabling the declarative expression of dataflows and handling the propagation of changes automatically.
Implicit parallelism for this process is supported as well.

#### Build environment and dependencies

So far, I've only tested building the framework on Microsoft Windows, with:
* Visual Studio 2013
* Intel C++ Compiler 14.0 in Visual Studio 2012/13

[Intel TBB 4.2](https://www.threadingbuildingblocks.org/) is a required dependency.
To compile and run the unit tests, the [Google test framework](https://code.google.com/p/googletest/) is required, too.

Cpp.React uses standard C++11 and the dependencies are portable, so other compilers/platforms should work, too.


### Feature overview

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

Event streams represent flows of discrete values as first-class objects, based on ideas found in [Deprecating the Observer pattern](infoscience.epfl.ch/record/176887/files/DeprecatingObservers2012.pdf). Example:

```C++
#include "react/EventStream.h"

using namespace react;

REACTIVE_DOMAIN(MyDomain);

auto leftClicked = MyDomain::MakeEventSource();
auto rightClicked = MyDomain::MakeEventSource();

auto clicked = leftClicked | rightClicked;

Observe(clicked, [] { cout << "button clicked!" << endl; });
```

#### Implicit parallelism

The change propagation is handled implicitly by a so called propagation engine.
Depending on the selected engine, independent propagation paths are automatically parallelized.
Pipelining of updates is supported as well.
For more details, see Propagation Engines.


### More examples

* [Examples](https://github.com/schlangster/cpp.react/blob/master/src/sandbox/Main.cpp)
* [Benchmark](https://github.com/schlangster/cpp.react/blob/master/src/benchmark/BenchmarkLifeSim.h)
* [Test cases](https://github.com/schlangster/cpp.react/tree/master/src/test)
