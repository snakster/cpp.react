# ![C++React](http://snakster.github.io/cpp.react//media/logo_banner3.png)

C++React is reactive programming library for C++11.

Generally speaking, it provides abstractions to handle change propagation and data processing for a push-based event model.
A more practical description is that it enables coordinated, multi-layered - _and potentially parallel_ - execution of callbacks.
All this happens implicitly, based on declarative definitions, with guarantees regarding

- _update minimality_ - nothing is re-calculated or processed unnecessarily;
- _glitch freedom_ - no transiently inconsistent data sets;
- _thread safety_ - no data races for parallel execution by avoiding side effects.

The core abstractions of the library are

- _signals_, reactive variables that are automatically re-calculated when their dependencies change, and
- _event streams_ as composable first class objects.

Signals specifically deal with aspects of time-varying state, whereas event streams facilitate event processing in general.

Additional features include

- a publish/subscribe mechanism for callbacks with side effects;
- a set of operations and algorithms to combine signals and events;
- a domain model to encapsulate multiple reactive systems;
- transactions to group related events, supporting both synchronous and asynchrounous execution.


## Documentation

[If you're interested in learning about C++React, have a look at its documentation.](http://snakster.github.io/cpp.react/)


## Using the library

This library is a work-in-progress. It should not be considered release quality yet and its API might still change.
It is, however, in a perfectly usable state and has already received a fair amount of testing and tuning.

### Dependencies

* [Intel TBB 4.2](https://www.threadingbuildingblocks.org/) (required)
* [Google test framework](https://code.google.com/p/googletest/) (optional, to compile the unit tests)
* [Boost 1.55.0 C++ Libraries](http://www.boost.org/) (optional, to include Reactor.h, which requires `boost::coroutine`)

### Compiling

C++React has been tested with the following compilers:

* Visual Studio 2013.2
* GCC 4.8.2
* Clang 3.4

To build with Visual Studio, use the pre-made solution found in `project/msvc/`.

To build with GCC or Clang, use [CMake](http://www.cmake.org/):
```
mkdir build
cd build
cmake ..
make
```

For more details, refer to the [Build instructions](https://github.com/snakster/cpp.react/wiki/Build-instructions).


## Features by example

### Signals

Signals are self-updating reactive variables.
They can be combined in expressions to create new signals, which are automatically re-calculated when their dependencies change.

```C++
using namespace std;
using namespace react;

// Defines a reactive domain that uses single-threaded, sequential updating
REACTIVE_DOMAIN(D, sequential)

// Defines aliases for types of the given domain,
// e.g. using VarSignalT<X> = VarSignal<D,X>
USING_REACTIVE_DOMAIN(D)

// Two reactive variables that can be manipulated imperatively
// to input external changes
VarSignalT<int> width  = MakeVar<D>(1);
VarSignalT<int> height = MakeVar<D>(2);

// A signal that depends on width and height and multiplies their values
SignalT<int> area = MakeSignal(
    With(width, height),
    [] (int w, int h) {
        return w * h;
    });
```
Signal values can be accessed imperatively:
```C++
cout << "area: " << area.Value() << endl; // => area: 2

// Width changed, so area is re-calculated automatically
width.Set(10);

cout << "area: " << area.Value() << endl; // => area: 20
```

Or, instead of using `Value()` to pull the new value, callback functions can be registered to receive notifications on a change:
```C++
Observe(area, [] (int newValue) {
	cout << "area changed: " << newValue << endl;
});
```

Overloaded operators for signal types allow to omit `MakeSignal` for a more concise syntax:
```C++
// Lift as reactive expression - equivalent to previous example
SignalT<int> area = width * height;
```

### Event streams

Unlike signals, event streams are not centered on changing state, but represent flows of discrete values. 
They are first-class objects and can be merged, filtered, transformed or composed to more complex types:

```C++
using namespace std;
using namespace react;

REACTIVE_DOMAIN(D, sequential)
USING_REACTIVE_DOMAIN(D)

// Two event sources
EventSourceT<Token> leftClick  = MakeEventSource<D>();
EventSourceT<Token> rightClick = MakeEventSource<D>();

// Merge both event streams
EventsT<Token> anyClick = leftClick | rightClick;

// React to events
Observe(anyClick, [] (Token) {
    cout << "clicked!" << endl;
});
```
```
leftClick.Emit(); // => clicked!
rightClick.Emit(); // => clicked!
```

### Parallelism and concurrency

When enabling it through the concurrency policy, updates are automatically parallelized:

```C++
REACTIVE_DOMAIN(D, parallel)

VarSignalT<int> in = MakeVar<D>(0);

SignalT<int> op1 = MakeSignal(in,
    [] (int in) {
        int result = doCostlyOperation1(in);
        return result;
    });

SignalT<int> op2 = MakeSignal(in,
    [] (int in) {
        int result = doCostlyOperation2(in);
        return result;
    });

// op1 and op2 can be re-calculated in parallel
SignalT<int> out = op1 + op2;
```

## Acknowledgements

The API of C++React has been inspired by the following two research papers:

* [Deprecating the Observer Pattern with Scala.React](http://infoscience.epfl.ch/record/176887/files/DeprecatingObservers2012.pdf)
* [REScala: Bridging Between Object-oriented and Functional Style in Reactive Applications](http://www.stg.tu-darmstadt.de/media/st/research/rescala_folder/REScala-Bridging-The-Gap-Between-Object-Oriented-And-Functional-Style-In-Reactive-Applications.pdf)
