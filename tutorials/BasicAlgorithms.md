---
layout: default
title: Algorithm basics
groups: 
 - {name: Home, url: ''}
 - {name: Tutorials , url: 'tutorials/'}
---
- [Converting between events and signals](#converting-between-events-and-signals)
- [Folding event streams into signals](#folding-event-streams-into-signals)
- [Synchronized signal access](#folding-event-streams-into-signals)

## Preface

We assume the following headers to be included for the coming examples:
{% highlight C++ %}
#include "react/Domain.h"
#include "react/Signal.h"
#include "react/Event.h"
#include "react/Observer.h"
#include "react/Algorithm.h"
{% endhighlight %}

Further, the following domain definition is used:
{% highlight C++ %}
REACTIVE_DOMAIN(D, sequential)
{% endhighlight %}


## Converting between events and signals

So far, signals and event streams have been viewed separately, but it's possible to combine them with a set of generic functions.

First, we use `Hold` to store the most recent event as a signal value:
{% highlight C++ %}
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
It should be noted, that signal observers are only notified if the signal value changes, hence the second `21` event doesn't produce output.

If a stream emits multiple events during a single turn, only the last one is stored by `Hold`:
{% highlight C++ %}
DoTransaction<D>([&] {
    mySensor.Samples << 20 << 21 << 21 << 22;
});
// output: 22
{% endhighlight %}

In the other direction, with `Monitor` we can generate an event stream from signal changes:
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
With `Iterate`, we can create signals that calculate their new value as a function of their current value.

This is demonstrated on the example of a reactive counter:
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


`Iterate` is semantically equivalent to the higher order functions fold/reduce you might know from other functional languages.
In this example, the signal is first initialized with zero;
then, for every received increment event, the given function is called with the event value (a token in this case) and the current value of the signal.
The return value of the function is used as the new signal value.

The event value can also be used in the computation. To show this, we calculate the average of measured samples:
{% highlight C++ %}
class Sensor
{
public:
    USING_REACTIVE_DOMAIN(D)

    EventSourceT<int>   Input = MakeEventSource<D,int>();
    
    SignalT<float> Average = Iterate(
        Input,
        0.0f,
        [] (int sample, float oldAvg) {
            return (oldAvg + sample) / 2.0f;
        });
};
{% endhighlight %}

`Iterate` could also be used to incrementally push event values into a `Signal<std::vector>`.

However, when considering performance, this could be problematic.
Updating a signal usually involves copying its current value, moving the copy into the passed function and comparing the result to the old value to decide whether it has been changed.
For some types, like `std::vector` in this case, these operations are rather expensive as they result in allocations and repeated element-wise copying and comparing.

To avoid this, `Iterate` also supports pass-by non-const reference. To enable this, we change the parameter type accordingly and return `void` instead of the new value:
{% highlight C++ %}
class Sensor
{
public:
    USING_REACTIVE_DOMAIN(D)

    EventSourceT<int>   Input = MakeEventSource<D,int>();
    
    SignalT<vector<int>> AllSamples = Iterate(
        Input,
        vector<int>{},
        [] (int input, vector<int>& v) {
            v.push_back(input);
        });

    SignalT<vector<int>> CriticalSamples = Iterate(
        Input,
        vector<int>{},
        [] (int input, vector<int>& v) {
            if (input > 10)
                v.push_back(input);
        });
};
{% endhighlight %}

Similar to `Signal.Modify()`, the downside is that since the new and old values can no longer be compared, the signal will always assume a change.


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
    vector<int>{},
    [] (int input, vector<int>& v, int threshold) {
        if (input > threshold)
            v.push_back(input);
    });
{% endhighlight %}

The current values of the signal pack constructed by `With(...)` are passed as additional arguments to the iteration function.
In this case, that's only `Threshold`.
Changes of the synchronized signals do _not_ lead to calls to iteration function.
It's still events from the first event stream argument that triggers them (in this case, that would be `Input`).