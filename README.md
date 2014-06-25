# Introduction
Cpp.React is reactive programming library for C++11.
Generally speaking, it provides abstractions to handle change propagation and data processing for a push-based event model.

The two core features of the library are

* **signals**, reactive variables that are automatically re-calculated when their dependencies change, and
* **event streams** as composable first class objects.

Signals specifically deal with aspects of time-varying state, whereas event streams facilitate generic processing.
A set of operations and algorithms can be used to combine signals and events.
Together, these abstractions enable intuitive modeling of dataflow systems in a declarative manner.

In this case, declarative means that the programer defines how values are calculated through functional composition of other values.
Applying these calcuations is implicitely handled by a so-called propagation engine as changes are propagated through the dataflow graph.

Since the engine has the "whole picture", it can schedule updates efficiently, so that:

* No value is re-calculated or processed unnecessarily;
* intermediate results are cached to avoid redundant calculations;
* no glitches will occur (i.e. no inconsistent sets of input values).

It can even implicitly parallelize calculations, while automatically taking care of potential data-races and effective utilization of available parallel hardware.

This makes using Cpp.React an alternative to implementing imperative change propagation manually through callback functions and side-effects.

# Documentation
The documentation is still incomplete, but it already contains plenty of useful information and examples.
It can be found in the [wiki](https://github.com/schlangster/cpp.react/wiki).

# Development
Cpp.React is a work-in-progress and should not be considered release quality yet.
The project has been actively developed for about 6 months and has seen a fair share of testing and tuning during that time, so it's in a usable state.

### Compiling
Cpp.React has been tested with the following compilers:

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

# Features by example

## Signals

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
// Signal values can be accessed imperativly at any time, but only VarSignals can be directly manipulated.
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

## Events and Observers

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

## Parallelism and concurrency

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

## More examples

* [Examples](https://github.com/schlangster/cpp.react/tree/master/examples/src)
* [Test cases](https://github.com/schlangster/cpp.react/tree/master/tests/src)
* [Benchmarks](https://github.com/schlangster/cpp.react/blob/master/benchmarks/src/BenchmarkLifeSim.h)

# Acknowledgements

The API of Cpp.React has been inspired by the following two research papers:

* [Deprecating the Observer Pattern with Scala.React](http://infoscience.epfl.ch/record/176887/files/DeprecatingObservers2012.pdf)
* [REScala: Bridging Between Object-oriented and Functional Style in Reactive Applications](http://www.stg.tu-darmstadt.de/media/st/research/rescala_folder/REScala-Bridging-The-Gap-Between-Object-Oriented-And-Functional-Style-In-Reactive-Applications.pdf)
