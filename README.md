# Introduction

Cpp.React is an experimental [Reactive Programming](http://en.wikipedia.org/wiki/Reactive_programming) library for C++11.

It provides abstractions to simplify the implementation of reactive behaviour.

To react to changing data, variables can be declared in terms of functions that calcuate their values, rather than manipulating them directly.
A runtime engine will automatically handle the change propagation by re-calculating data if its dependencies have been modified.

To react to events, event streams are provided as composable first class objects with value semantics.

For seamless integration with existing code, _reactive domains_ encapsulate a reactive sub-system, with an input interface to trigger changes imperatively, and an output interface to apply side effects.
Inside of a reactive domain functional purity is maintained, which allows the easier reasoning about program behaviour and implicit parallelism.

As an alternative to implementing change propagation or callback-based approaches manually, Cpp.React offers the following benefits:
* Less boilerplate code;
* consistent updating without redundant calculations or glitches;
* implicit parallelism.

# Documentation
The documentation is still incomplete, but it already contains plenty of useful information and examples.
It can be found in the [wiki](https://github.com/schlangster/cpp.react/wiki).

# Development
This library is still work-in-progress and should not be considered release quality yet.
That being said, it's in a usable state, has been actively developed during the last 6 months and has seen a fair share of testing and tweaking during that time.

### Compiling
So far, Cpp.React has only been tested in Visual Studio 2013 as that's the development environment used to create it.

You are welcome to try compiling it with other C++11 compilers/on other platforms and report any issues you encounter.

### Projects
The VS solution currently contains four pojects:

* CppReact - The library itself.
* CppReactBenchmark - A number of benchmarks used to compare the different propagation strategies.
* CppReactTest - The unit tests.
* CppReactSandbox - A project containing several basic examples. You can use this to start experimenting with the library.

### Dependencies
Cpp.React uses several external dependencies, but only one of them is mandatory:

* [Intel TBB 4.2](https://www.threadingbuildingblocks.org/) (required)
* [Google test framework](https://code.google.com/p/googletest/) (optional, to compile the unit tests)
* [Boost C++ Libraries](http://www.boost.org/) (optional, to use ReactiveLoop, which requires boost::coroutine)

TBB is required, because it enables the parallel propagation strategies.
Future plans are to separate the multi-threaded and single-threaded propagation engines more cleanly to remove that dependency if parallelism is not used.

# Features by example

## Signals

Signals are self-updating reactive variables.
They can be combined to expressions to create new signals, which are automatically re-calculated whenever one of their data dependencies changes.

```C++
using namespace std;
using namespace react;

REACTIVE_DOMAIN(D);

// Two reactive variables that can be manipulated imperatively
D::VarSignalT<int> width  = D::MakeVar(1);
D::VarSignalT<int> height = D::MakeVar(2);

// A signal that depends on width and height and multiplies their values
D::SignalT<int> area = MakeSignal(With(width, height),
    [] (int w, int h) {
        return w * h;
    });

cout << "area: " << area.Value() << endl; // => area: 2

// Width changed, so area is re-calculated automatically
width.Set(10);

cout << "area: " << area.Value() << endl; // => area: 20
```

When used with built-in operators, this can be simplified further:
```C++
D::SignalT<int> area = width * height;
```

## Events and Observers

Event streams represent flows of discrete values.
They are first-class objects and can be merged, filtered, transformed or composed to more complex types:

```C++
using namespace std;
using namespace react;

REACTIVE_DOMAIN(D);

// Two event sources
D::EventSourceT<Token> leftClicked  = D::MakeEventSource();
D::EventSourceT<Token> rightClicked = D::MakeEventSource();

// Merge both event streams
D::EventsT<Token> merged = leftClicked | rightClicked;

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
    REACTIVE_DOMAIN(D, ToposortEngine<sequential>);

    auto a = D::MakeVar(1);
    auto b = D::MakeVar(2);
    auto c = D::MakeVar(3);

    auto x = (a + b) * c;

    b <<= 20;
}

// Parallel updating
{
    REACTIVE_DOMAIN(D, ToposortEngine<parallel>);

    auto in = D::MakeVar(0);

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

* [Examples](https://github.com/schlangster/cpp.react/blob/master/src/sandbox/Main.cpp)
* [Test cases](https://github.com/schlangster/cpp.react/tree/master/src/test)
* [Benchmark](https://github.com/schlangster/cpp.react/blob/master/src/benchmark/BenchmarkLifeSim.h)

# Acknowledgements

The API of Cpp.React has been inspired by the following two research papers:
* [Deprecating the Observer Pattern with Scala.React](http://infoscience.epfl.ch/record/176887/files/DeprecatingObservers2012.pdf)
* [REScala: Bridging Between Object-oriented and Functional Style in Reactive Applications](http://www.stg.tu-darmstadt.de/media/st/research/rescala_folder/REScala-Bridging-The-Gap-Between-Object-Oriented-And-Functional-Style-In-Reactive-Applications.pdf)
