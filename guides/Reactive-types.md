---
layout: default
title: Reactive types
groups: 
 - {name: Home, url: ''}
 - {name: Guides , url: 'guides/'}
---

<!--
# Motivation
The [graph model guide]() explained how reactive values form a graph, with data being propagated along its edges.
It did not concretize what "value" or "data" actually means, because the dataflow perspective allows to abstract from such details.

In this guide, the level of abstraction is lowered to introduce the core reactive types that make up this library.
-->

The purpose of this guide is to state the [Motivation](#motivation) behind this library and to introduce the core reactive types it provides.
Namely those are:

- [Signals](#signals);
- [Event streams](#event-streams);
- [Observers](#observers).

The [Conclusion](#conclusion) explains how it all fits together.

# Motivation

Reacting to events is a common task in modern applications, for instance to respond to user input.
Often events themselves are defined and triggered by a framework, but handled in client code.
Callback mechanisms are used to implement this. They allow clients to register and unregister functions at runtime,
and have these functions called when specific events occur.

Conceptionally, there is nothing wrong with this approach, but problems manifest when attempting to distribute complex business logic across multiple callbacks.
The three main reasons for this are:

- (1) The control flow is "scattered"; events may be occur at arbitrary times, callbacks may be added and removed on-the-fly, etc.
- (2) Data is exchanged via shared state and side-effects.
- (3) Callback execution is uncoordinated; callbacks may trigger other callbacks etc.

The combination of these points makes it increasingly difficult to reason about program behaviour and properties like correctness or algorithmic complexity.
Further, it complicates debugging and when adding concurrency to the mix, the situation gets worse.

Decentralized control flow is inherent to the creation of interative applications, but usage of side-effects and uncoordinated execution can be addressed.
This is what this library - and reactive programming in general - does.


# Signals

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
Their main advantage is scalability w.r.t. to performance and complexity.
When when considering deeply nested dependency relations involving computationally expensive operations,
signals apply selective updating, as if only the necessary values were re-calculated imperatively, while allowing to define calculations without side effects


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
This is possible, because event streams are first-class values.


## Observers

Observers fill the role of "common" callbacks with side-effects.
They allow to register functions to subjects, which can be any signal or event stream, and have these functions called when notified by the subject.
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

Another example, this time with a signal observer:
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


# Conclusion

The strength of this design are as follows:

There are two key points:

- First-class objects

- Avoidance of side-effects

  Compare this to representing events on API level, i.e. `RegisterLeftClickCallback` vs `EventsT<> LeftClick`.

- Fine-grained ab


## Further reading

The concept of signals and event streams are established concepts from reactive programming and not original to this library.
An academic paper which describes them well and has been especially influential for the design of this library is [Deprecating the Observer Pattern](http://lamp.epfl.ch/~imaier/pub/DeprecatingObserversTR2010.pdf) by Maier et al.