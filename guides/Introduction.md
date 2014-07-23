---
layout: default
title: Introduction to C++React
groups: 
 - {name: Home, url: ''}
 - {name: Guides , url: 'guides/'}
---

- [Motivation](#signals)
- [Design outline](#signals)
- [Signals](#signals)
- [Event streams](#event-streams)
- [Observers](#observers)
- [Coordinated change propagation](#coordinated-change-propagation)
- [Conclusion](#conclusion)


## Motivation

Reacting to events is a common task in modern applications, for example to respond to user input.
Often events themselves are defined and triggered by a framework, but handled in client code.
Callback mechanisms are typically used to implement this.
They allow clients to register and unregister functions at runtime, and have these functions called when specific events occur.

Conceptionally, there is nothing wrong with this approach, but problems can arise when distributing complex program logic across multiple callbacks.
The three main reasons for this are:

- The control flow is "scattered"; events may be occur at arbitrary times, callbacks may be added and removed on-the-fly, etc.
- Data is exchanged via shared state and side effects.
- Callback execution is uncoordinated; callbacks may trigger other callbacks etc. and change propagation follows a "flooding" scheme.

In combination, these factors make it increasingly difficult to reason about program behaviour and properties like correctness or algorithmic complexity.
Further, debugging is difficult and when adding concurrency to the mix, the situation gets worse.

Decentralized control flow is inherent to the creation of interative applications, but issues of shared state and uncoordinated execution can be addressed.
This is what C++React - and reactive programming in general - does.


## Design outline

The issue of uncoordinated callback execution is handled by adding another layer of intelligence between triggering and actual execution.
This layer schedules callbacks which are ready to be executed, potentially using multiple threads, while guarenteeing certain safety and complexity properteries.

The aforementioned usage of shared state and side effects is employed due to a lack of alternatives to implement dataflow between callbacks.
Thus, to improve the situation, proper abstractions to model dataflow explicitly are needed.

From a high-level perspective, this dataflow model consists of entities, which can emit and/or receive data, and pure functions to "wire" them together.
Instead of using side effects, these functions pass data through arguments and return values, based on semantics of the connected entities.
There exist multiple types of entities, representing different concepts like time changing values, event occurrences or actions.

Essentially, this means that callbacks are chained and can pass data in different ways, all of which is done in a composable manner, backed by a clear semantic model.

The following sections will introduce the core abstractions in more detail.


## Signals

Signals are used to model dependency relations between mutable values.
A `SignalT<S>` instance represents a container holding a single value of type `S`, which will notify dependents when that value changes.
Dependents could be other signals, which will re-calculate their own values as a result.

To explain this by example, consider the following expression:
{% highlight C++ %}
X = A + B
{% endhighlight %}

This could be interpreted as an imperative statement:
{% highlight C++ %}
int X = A + B;
{% endhighlight %}

If `A` or `B` might change later, the statement has to be re-executed to uphold the invariant of `X` being equal to the sum of `A` and `B`.

Alternatively, instead of storing `X` as a variable in memory, it could be defined as a function:
{% highlight C++ %}
int X() { return A + B; }
{% endhighlight %}
This removes the need to update `X` imperatively, but also repeats the calculation on every access rather than on changes only.

Signals are implemented as a combination of both approaches.
To demonstrate this, `A` and `B` are now declared as `SignalT<int>`.
A function is defined, which calculates the value of `X` based on its given arguments:
{% highlight C++ %}
int CalcX(int a, int b) { return a + b; }
{% endhighlight %}

This function is used to connect `X` to its dependencies `A` and `B`:
{% highlight C++ %}
SignalT<int> X = MakeSignal(With(A, B), CalcX);
{% endhighlight %}
The values of `A` and `B` are passed as arguments to `CalcX` and the return value is used as value of `X`.

`CalcX` is automatically called to set the initial value of `X`, or upon notification by one of its dependencies about a change.
If the new value of `X` is different from the current one, it would send change notifications to its own dependents (if it had any).

A special type of signals are `VarSignals`.
Instead of automatically calculating their values based on other signals, they can be changed directly.
This enables interaction with signals from imperative code. Here's the complete example:
{% highlight C++ %}
VarSignalT<int> A = MakeVar(1);
VarSignalT<int> B = MakeVar(2);
SignalT<int>    X = MakeSignal(With(A, B), CalcX);
{% endhighlight %}
{% highlight C++ %}
cout << X.Value() << endl; // output: 3
A.Set(2);
cout << X.Value() << endl; // output: 4
{% endhighlight %}

Updates happen pushed-based, i.e. as part of `A.Set(2)`.
`Value()` is a pull-based interface to retrieve the stored result.

In summary, the value of signal is calculated by a function, stored in memory, and automatically updated when its dependencies change by re-applying that function with new arguments.
When considering deeply nested dependency relations involving computationally expensive operations,
signals apply selective updating, as if only the necessary values were re-calculated imperatively, while the calculations themselves are defined as pure functions.


## Event streams

Unlike signals, event streams do not represent a single, time-varying and persistent value, but streams of fleeting, discrete values.
This allows to model general events like mouse clicks or values from a hardware sensor, which do not fit the category of state changes as covered by signals.
Event streams and signals are similar in the sense that both are reactive containers that can notify their dependents.

An event stream of type `Events<E>` propagates a list of values `E` and will notify dependents, if that list is non-empty.
Dependents can then process the propagated values, after which they are discarded.
In other words, event streams don't hold on to values, they only forward them.

Here's an example:
{% highlight C++ %}
EventSourceT<int> A  = MakeEventSource<int>();
EventSourceT<int> B  = MakeEventSource<int>();

EventsT<int>	  X  = Merge(A, B);

EventsT<int>	  Y1 = Filter(X, [] (int v) { return v > 100; });
EventsT<int>	  Y2 = Filter(X, [] (int v) { return v < 10; });

EventsT<float>	  Z  = Transform(Y1, [] (int v) { return v / 100.0f; }]);
...
{% endhighlight %}
{% highlight C++ %}
A.Push(1);
B.Push(2);
...
{% endhighlight %}

`EventSourceT` exists analogously to `VarSignalT` to provide an imperative interface.
Values enter through sources `A` and `B`, both of which get merged into a single stream `X`.
`Y1` and `Y2` are each filtered to contain only certain numbers forwarded from `X`.
`Z` takes values from `Y1` and transforms them.

Of course, for those events to have an effect, they should trigger actions or be stored somewhere persistently.
The example leaves that part out for now and instead focuses on composability, i.e. how functions like `Merge` or `Transform` are used to create new event streams from existing ones.
This is possible, because event streams are first-class values, as opposed to being indirectly represented by the API (e.g. `OnMouseClick()` vs. `EventsT<> MouseClick`).

## Observers

Observers fill the role of "common" callbacks with side effects.
They allow to register functions at subjects, which can be any signal or event stream, and have these functions called when notified by the subject.
In particular, a signal observer is called every time the signal value changes, and an event observer is called for every event that passes the stream.
Observers are only ever on the receiving side of notifications, because they have no dependents.

Similar to signals and event streams, observers are first-class objects.
They're created with `Observe(theSubject, theCallback)`, which returns an instance of `ObserverT`.
This instance provides an interface to detach itself from the subject explicitly.

An example shows how observers complement event streams to trigger actions (e.g. console output):
{% highlight C++ %}
EventsT<> LeftClick  = ...;
EventsT<> RightClick = ...;
// Note: If the event value type is omitted, the type 'Token' is used as default.
{% endhighlight %}
{% highlight C++ %}
ObserverT obs =
	Observe(
		Merge(LeftClick, RightClick),
		[] (Token) {
			cout << "Clicked" << endl;
		});
...
{% endhighlight %}

Another example, this time using a signal observer:
{% highlight C++ %}
VarSignal<int> LastValue  = ...;

{% endhighlight %}
{% highlight C++ %}
ObserverT obs =
	Observe(
		LastValue,
		[] (int v) {
			cout < "Value changed to " << v << endl;
		});
...
{% endhighlight %}

The obvious benefit is that we don't have to implement callback registration and dispatch mechanisms ourselves.
Further, they are already in place and available on every reactive value without any additional steps.


## Coordinated change propagation

A problem that has been mentioned earlier is the uncoordinated execution of callbacks.
To construct a practical scenario for this, imagine a graphical user interface, consisting of multiple components that are positioned on the screen.
If the size of any of the components changes, the screen layout has to be updated.
There exist several controls the user can interact with to change the size of specific components.

In summary, this scenario defines three layers, connected by callbacks:

- the input controls;
- the display components;
- the screen layout.

The question is, what happens if a single control affects multiple components:

- Each component individually triggers an update of the layout. As we add more layers to our scenario, the number of updates caused by a single change might grow exponentially.
- Some of these updates are executed while part of the components have already been changed, while others have not. This can lead to intermittent errors - or _glitches_ - which are hard to spot.
- Parallelization requires mutually exclusive acccess to shared data and critical operations (i.e. updating the layout cannot happen concurrently).

When building the same system based on signals, events and observers, execution of individual callback functions is ordered.
First, all inputs are processed; then, the components are changed; lastly, the layout is updated once.
This ordering is based on the inherent hierarchy of the presented model.

Enabling parallelization comes naturally with approach, as it becomes an issue of implicitly synchronizing forks and joins of the control flow rather than managing mutual exclusive access to data.


## Conclusion

The presented reactive types provide us with specialized tools to address requirements that would otherwise be implemented with callbacks and side effects:

- Signals, as an alternative to updating and propagating state changes manually.
- Event streams, as an alternative to transfering data between event handlers explicitly, i.e. through shared message queues.

For cases where callbacks with side effects are not just a means to an end, but what is actually intended, observers exist as an alternative to setting up registration mechanisms by hand.

Change propagation is not just based on flooding, but uses a coordinated approach for

- avoidance of intermittent updates;
- glitch freedom;
- implicit parallelism.

This concludes an overview of the main features C++React has to offer.


### Further reading

The concepts described in this article are well-established in reactive programming and not original to this library, though semantics may slightly differ between implementations.
An academic paper which has been especially influential for the design of this library is [Deprecating the Observer Pattern](http://lamp.epfl.ch/~imaier/pub/DeprecatingObserversTR2010.pdf) by Maier et al.