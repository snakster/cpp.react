# ![C++React](http://schlangster.github.io/cpp.react//media/logo_banner3.png)

C++React is reactive programming library for C++11.

Generally speaking, it provides abstractions to handle change propagation and data processing for a push-based event model.
A more practical description is that it enables coordinated, multi-layered - and potentially parallel - execution of callbacks.
All this happens implicitly, based on declarative definitions, with guarantees regarding

- update-minimality - nothing is re-calculated or processed unnecessarily;
- glitch-freedom - no transiently inconsistent data sets;
- thread-safety - no data races for parallel execution.

The core abstractions of the library are

- **signals**, reactive variables that are automatically re-calculated when their dependencies change, and
- **event streams** as composable first class objects.

Signals specifically deal with aspects of time-varying state, whereas event streams facilitate event processing in general.

Additional features include

- a publish/subscribe mechanism for callbacks with side-effects;
- a set of operations and algorithms to combine signals and events;
- a domain model to encapsulate multiple reactive systems;
- transactions to group related events, supporting both synchronous and asynchrounous execution.


## Documentation

If you're interested in learning about C++React, [have a look at its documentation](http://schlangster.github.io/cpp.react/).

It's still incomplete, but it already contains plenty of useful information and examples.


## Development

This library is still a work-in-progress and should not be considered release quality yet.

Don't let this statement stop you from giving it a try though!
It's in a usable state and has already seen a fair share of testing and tuning during it's development so far.


### Compiling

The following compilers are supported at the moment:

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

For more details, see [Build instructions](https://github.com/schlangster/cpp.react/wiki/Build-instructions).


### Dependencies

* [Intel TBB 4.2](https://www.threadingbuildingblocks.org/) (required)
* [Google test framework](https://code.google.com/p/googletest/) (optional, to compile the unit tests)
* [Boost 1.55.0 C++ Libraries](http://www.boost.org/) (optional, to include Reactor.h, which requires `boost::coroutine`)


## Features by example

### Signals

Signals are self-updating reactive variables.
They can be combined to expressions to create new signals, which are automatically re-calculated whenever one of their data dependencies changes.

```C++
using namespace std;
using namespace react;

// Define a reactive domain that uses single-threaded, sequential updating
REACTIVE_DOMAIN(D, sequential)

// Define aliases for types of the given domain,
// e.g. using VarSignalT<X> = VarSignal<D,X>
USING_REACTIVE_DOMAIN(D)

// Two reactive variables that can be manipulated imperatively
VarSignalT<int> width  = MakeVar<D>(1);
VarSignalT<int> height = MakeVar<D>(2);

// A signal that depends on width and height and multiplies their values
SignalT<int> area = MakeSignal(
    With(width, height),
    [] (int w, int h) {
        return w * h;
    });
```
```
// Signal values can be accessed imperativly at any time,
// but only VarSignals can be directly manipulated.
cout << "area: " << area.Value() << endl; // => area: 2

// Width changed, so area is re-calculated automatically
width.Set(10);

cout << "area: " << area.Value() << endl; // => area: 20
```

Overloaded operators for signal types allow to omit `MakeSignal` in this case for a more concise syntax:
```C++
// Lift as reactive expression - equivalent to previous example
SignalT<int> area = width * height;
```

### Event streams and Observers

Event streams represent flows of discrete values. They are first-class objects and can be merged, filtered, transformed or composed to more complex types:

```C++
using namespace std;
using namespace react;

REACTIVE_DOMAIN(D, sequential)
USING_REACTIVE_DOMAIN(D)

// Two event sources
EventSourceT<Token> leftClicked  = MakeEventSource<D>();
EventSourceT<Token> rightClicked = MakeEventSource<D>();

// Merge both event streams
EventsT<Token> merged = leftClicked | rightClicked;

// React to events
auto obs = Observe(merged, [] (Token) {
    cout << "clicked!" << endl;
});
```
```
rightClicked.Emit(); // => clicked!
```

### Parallelism and concurrency

The change propagation is handled implicitly by a propagation engine.
Depending on the selected engine, updates can be parallelized:

```C++
using namespace react;


// Sequential updating
{
    REACTIVE_DOMAIN(D, sequential)

    auto a = MakeVar<D>(1);
    auto b = MakeVar<D>(2);
    auto c = MakeVar<D>(3);

    auto x = (a + b) * c;

    b <<= 20;
}

// Parallel updating
{
    REACTIVE_DOMAIN(D, parallel)

    auto in = MakeVar<D>(0);

    auto op1 = MakeSignal(in, [] (int in)
    {
        int result = in /* Costly operation #1 */;
        return result;
    };

    auto op2 = MakeSignal(in, [] (int in)
    {
        int result = in /* Costly operation #2 */;
        return result;
    };

    auto out = op1 + op2;

    // op1 and op2 can be re-calculated in parallel
    in <<= 123456789;
}
```

### More examples

* [Examples](https://github.com/schlangster/cpp.react/tree/master/examples/src)
* [Test cases](https://github.com/schlangster/cpp.react/tree/master/tests/src)
* [Benchmarks](https://github.com/schlangster/cpp.react/blob/master/benchmarks/src/BenchmarkLifeSim.h)

## Acknowledgements

The API of Cpp.React has been inspired by the following two research papers:

* [Deprecating the Observer Pattern with Scala.React](http://infoscience.epfl.ch/record/176887/files/DeprecatingObservers2012.pdf)
* [REScala: Bridging Between Object-oriented and Functional Style in Reactive Applications](http://www.stg.tu-darmstadt.de/media/st/research/rescala_folder/REScala-Bridging-The-Gap-Between-Object-Oriented-And-Functional-Style-In-Reactive-Applications.pdf)
