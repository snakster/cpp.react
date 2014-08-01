---
layout: default
title: Algorithms
groups: 
 - {name: Home, url: ''}
 - {name: Tutorials , url: 'tutorials/'}
---
- [Converting between events and signals](#converting-between-events-and-signals)
- [Folding event streams into signals](#folding-event-streams-into-signals)
- [Synchronized signal access](#folding-event-streams-into-signals)

## Converting between events and signals

So far, signals and event streams have been viewed separately, but it's possible to combine them with a set of generic functions.

First, we use `Hold` to store the most recent event as a signal value:
{% highlight C++ %}
#include "react/Algorithm.h"
// Note: Include other react headers as required

REACTIVE_DOMAIN(D, sequential)

class Sensor
{
public:
    USING_REACTIVE_DOMAIN(D)

    EventSourceT<int>   Samples     = MakeEventSource<int>();
    SignalT<int>        LastSample  = Hold(Samples);
};
{% endhighlight %}
{% highlight C++ %}
Sensor mySensor;

Observe(mySensor.LastSample, [] (int v) {
    cout << v << endl;
});
{% endhighlight %}
{% highlight C++ %}
mySensor.Samples << 20 << 21 << 21 << 22;
// output: 20, 21, 22
{% endhighlight %}
Note that signal observers are only notified if the signal value changes, hence the second `21` event doesn't produce output.

If a stream emits multiple events during a single turn, only the last one is stored by `Hold`:
{% highlight C++ %}
DoTransaction<D>([&] {
    mySensor.Samples << 20 << 21 << 21 << 22;
});
// output: 22
{% endhighlight %}

To convert in the opposite direction, `Monitor` generates an event stream from signal changes:
{% highlight C++ %}
class Entity
{
public:
    USING_REACTIVE_DOMAIN(D)

    VarSignalT<int>   XPos = MakeVar(0);
    VarSignalT<int>   YPos = MakeVar(0);

    EventsT<>         DimensionChanged = Tokenize(Monitor(XPos) | Monitor(YPos));
};
{% endhighlight %}

This is similar to observing a signal, but by turning the changes into an event stream, we can process them further.


## Folding event streams into signals

While signals are stateful in the sense that they store their current value,
all dependent signals (i.e. not `VarSignals`) could be expressed as pure functions of their predecessors.
The `Iterate` function allows us to create signals that use their current state to calculate the next value.

A simple application that makes use of this is a signal-based counter:
{% highlight C++ %}
class Counter
{
public:
    USING_REACTIVE_DOMAIN(D)

    EventSourceT<>  Increment = MakeEventSource<D>();
    
    SignalT<int> Count = Iterate(
        Increment,
        0,
        [] (Token, int oldCount) {
            return oldCount + 1;
        });
};
{% endhighlight %}
{% highlight C++ %}
Counter myCounter;

// Note: Using function-style operator() instead of Emit() and Value()

myCounter.Increment();
myCounter.Increment();
myCounter.Increment();

cout << myCounter.Count() << endl;
 // output: 3
{% endhighlight %}

`Iterate` is semantically equivalent to the higher order functions fold/reduce known from other functional languages.
In this example, the signal is first initialized with zero;
then, for every received increment event, the given iteration function is called with the event value (a token in this case) and the current value of the signal.
The return value of the function is used as the new signal value.

The event value can also be used in the computation. To show this, we calculate the sum of measured samples:
{% highlight C++ %}
class Sensor
{
public:
    USING_REACTIVE_DOMAIN(D)

    EventSourceT<int> Input = MakeEventSource<D,int>();
    
    SignalT<int> Sum = Iterate(
        Input,
        0,
        [] (int sum, int input) {
            return input + sum;
        });
};
{% endhighlight %}

`Iterate` could also be used to incrementally push event values into a `Signal<std::vector>`.

However, when considering performance, this could be problematic.
Updating a signal usually involves copying its current value, moving the copy into the passed function and comparing the result to the old value to decide whether it has been changed.
For some types, e.g. `std::vector`, these operations are rather expensive as they result in allocations and repeated element-wise copying and comparing.

To avoid this, `Iterate` also supports pass-by non-const reference. To enable this, we change the parameter type accordingly and return `void` instead of the new value:
{% highlight C++ %}
class Sensor
{
public:
    USING_REACTIVE_DOMAIN(D)

    EventSourceT<int>   Input = MakeEventSource<D,int>();
    
    SignalT<vector<int>> AllSamples = Iterate(
        Input,
        vector<int>{ },
        [] (int input, vector<int>& v) {
            v.push_back(input);
        });

    SignalT<vector<int>> CriticalSamples = Iterate(
        Input,
        vector<int>{ },
        [] (int input, vector<int>& v) {
            if (input > 10)
                v.push_back(input);
        });
};
{% endhighlight %}

Similar to `Signal.Modify()`, the downside is that since the new and old values can no longer be compared, hence the signal will always assume a change.


## Synchronized signal access

What if logic inside the `Iterate` function requires the current values of other signals?

For example, the critical threshold from the last example could be a signal instead of a hardcoded value:
{% highlight C++ %}
class Sensor
{
public:
    USING_REACTIVE_DOMAIN(D)

    EventSourceT<int>   Input       = MakeEventSource<D,int>();
    Signal<int>         Threshold   = MakeSignal<D>(10);

    // ...
};
{% endhighlight %}

We could just use the `Value()` accessor of `Threadhold`:
{% highlight C++ %}
// Note: This is potentially unsafe
SignalT<vector<int>> CriticalSamples = Iterate(
    Input,
    vector<int>{},
    [] (int input, vector<int>& v) {
        if (input > Threshold.Value())
            v.push_back(input);
    });
{% endhighlight %}

This approach introduces two issues:

- For single-threaded updating, even if `Threshold` were to change in the same turn, we might read the value too early. In other words, this is prone to glitches.
- For multi-threaded updating, calling `Value()` leads to data-races.

The proper way of accessing `Threshold` is by passing an additional signal pack to `Threshold`:
{% highlight C++ %}
// Note: This is potentially unsafe
SignalT<vector<int>> CriticalSamples = Iterate(
    Input,
    With(Threshold),
    vector<int>{ },
    [] (int input, vector<int>& v, int threshold) {
        if (input > threshold)
            v.push_back(input);
    });
{% endhighlight %}

The current values of the signal pack constructed by `With(...)` are passed as additional arguments to the iteration function.
Changes of the synchronized signals do _not_ lead to calls to iteration function.
It's still events from the first event stream argument that triggers them (in this case, that would be `Input`).

Besides `Iterate`, there are more functions that support synchronized access to signal values. Namely those are

* `Observe` for event streams,
* `Filter`, and
* `Transform`.

Here's another example for `Observe`:
{% highlight C++ %}
class Person
{
public:
    USING_REACTIVE_DOMAIN(D)

    VarSignalT<string>  Name;
    VarSignalT<string>  Occupation;
    VarSignalT<int>     Age;

    Person(string name, string occupation, int age) :
        Name( MakeVar<D>(name) ),
        Occupation( MakeVar<D>(occupation) ),
        Age( MakeVar(age) )
    {}
};
{% endhighlight %}

{% highlight C++ %}
Person joe( "Joe", "Plumber", 42 );

auto obs = Observe(
    Monitor(joe.Age),
    With(joe.Name, joe.Occupation),
    [] (int age, string name, string occupation) {
        cout << name << " the " << occupation << " turned " << age << endl;
    });

joe.Age <<= joe.Age.Value() + 1;
// output: Joe the Plumber turned 43
{% endhighlight %}