# Introduction
Cpp.React is an experimental [Reactive Programming](http://en.wikipedia.org/wiki/Reactive_programming) library for C++11.

It provides abstractions to simplify the implementation of reactive behaviour.

To react to changing data, reactive variables are declared in terms of functions that calcuate their values, rather than manipulating them directly.
A runtime engine will automatically handle the change propagation by re-calculating data if its dependencies have been modified.

To react to events, event streams are provided as composable first class objects with value semantics.

For seamless integration with existing code, _reactive domains_ encapsulate a reactive sub-system, with an input interface to trigger changes imperatively, and an output interface to apply side effects.

As an alternative to implementing change propagation or callback-based approaches manually, using Cpp.React results in less boilerplate code and better scalability due to the guarentees of avoiding unnecessary recalculations and glitches.
Furthermore, since functional purity is maintained in each reactive domain, the propagation engine can safely support implicit parallelism for the updating process.
Behind the scenes, task-based programming and dynamic chunking based on collected timing data are employed to optimize parallel utilization.

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
Cpp.React uses several external dependencies, but only one of them is mandatory:

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

REACTIVE_DOMAIN(D)
USING_REACTIVE_DOMAIN(D)

// Two reactive variables that can be manipulated imperatively
VarSignalT<int> width  = MakeVar<D>(1);
VarSignalT<int> height = MakeVar<D>(2);

// A signal that depends on width and height and multiplies their values
SignalT<int> area = MakeSignal(With(width, height),
    [] (int w, int h) {
        return w * h;
    });

cout << "area: " << area.Value() << endl; // => area: 2

// Width changed, so area is re-calculated automatically
width.Set(10);

cout << "area: " << area.Value() << endl; // => area: 20
```

For expressions that use operators only, `MakeSignal` can be omitted completely:
```C++
// Lift as reactive expression
SignalT<int> area = width * height;
```

## Events and Observers

Event streams represent flows of discrete values.
They are first-class objects and can be merged, filtered, transformed or composed to more complex types:

```C++
using namespace std;
using namespace react;

REACTIVE_DOMAIN(D)
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

// ...

rightClicked.Emit(); // => clicked!
```

## Implicit parallelism

The change propagation is handled implicitly by a _propagation engine_.
Depending on the selected engine, updates can be parallelized:

```C++
using namespace react;


// Sequential updating
{
    REACTIVE_DOMAIN(D, ToposortEngine<sequential>)

    auto a = MakeVar<D>(1);
    auto b = MakeVar<D>(2);
    auto c = MakeVar<D>(3);

    auto x = (a + b) * c;

    b <<= 20;
}

// Parallel updating
{
    REACTIVE_DOMAIN(D, ToposortEngine<parallel>)

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
