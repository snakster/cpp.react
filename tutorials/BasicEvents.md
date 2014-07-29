---
layout: default
title: Event streams
groups: 
 - {name: Home, url: ''}
 - {name: Tutorials , url: 'tutorials/'}
---

- [Hello world](#hello-world)
- [Merging event streams](#merging-event-streams)
- [Processing events](#filtering-events)
- [Changing multiple inputs](#changing-multiple-inputs)

## Hello World

The previously shown signals hold state and push changes to their dependents.
Event streams, on the other hand, are used to model more generic event types that are not coupled to state, for example mouse clicks.

We start the tutorial by creating an `EventSource` that can emit strings:
{% highlight C++ %}
#include "react/Domain.h"
#include "react/Event.h"

REACTIVE_DOMAIN(D, sequential)
USING_REACTIVE_DOMAIN(D)

EventSourceT<string> mySource = MakeEventSource<D,string>();
{% endhighlight %}
`EventSource` and  `Events` are the respective counterparts of `VarSignal` and `Signal`.

Analogously to `VarSignalT<S>`, `EventSourceT<E>` is an alias for `EventSource<D,E>` defined by `USING_REACTIVE_DOMAIN`.

Unlike signals, event streams are purely push-based.
They forward values to be processed, but don't hold on to them.
There is no equivalent to the `Value()` accessor of `Signal`.

This means to do anything useful with `mySource`, we have to add an observer:

{% highlight C++ %}
Observe(mySource, [] (string s) {
    cout << s << endl;
});
{% endhighlight %}
{% highlight C++ %}
mySource.Emit(string( "Hello world" ));

// ... or with operator
mySource << string( "Hello world" );
{% endhighlight %}

Note that here we use the conventional stream operator `<<`, rather than `<<=`, which is used for signals.
The reasoning behind this is that event input is propagation-only, whereas signal input is both assignment and propagation.
In other words: Signals hold a value, event streams don't. The different operators are meant to symbolize that.

There's a third syntactic alternative that treats `mySource` as a function object:
{% highlight C++ %}
mySource(string( "Hello world" ));
{% endhighlight %}

These syntactic alternatives allow to distingish between several use cases.
For instance, if an event is used like a function triggering an action, function-style is appropriate.
If it acts more like a data stream, we can use the stream syntax.

It's not uncommon that the value type transported by an event stream is irrelevant and we are only interested in the fact that it occurred.
For instance, when a button has been clicked. In this case, the value type can be omitted.
We create a second version of the "Hello world" program to demonstrate this:
{% highlight C++ %}
EventSourceT<> helloWorldTrigger = MakeEventSource<D>();
{% endhighlight %}
{% highlight C++ %}
Observe(helloWorldTrigger, [] (Token) {
    cout << "Hello world" << endl;
});
{% endhighlight %}
{% highlight C++ %}
helloWorldTrigger.Emit();

// Stream-style:
helloWorldTrigger << Token::value;

// Function-style:
helloWorldTrigger();
{% endhighlight %}

Internally, the value transported by token streams is of type `Token`, hence for the observer function, we added an unnamed argument of this type.


## Merging event streams

From what we've seen so far, event streams are little more than callback registries;
but their true strength is composability.

For example, lets say we have two event sources that represent different mouse buttons.
We can easily merge them to a single stream that contains events from both buttons:
{% highlight C++ %}
EventSourceT<> leftClick  = MakeEventSource<D>();
EventSourceT<> rightClick = MakeEventSource<D>();

EventsT<>      anyClick   = Merge(leftClick, rightClick);
{% endhighlight %}
{% highlight C++ %}
Observe(anyClick, [] (Token) {
    cout << "button clicked" << endl;
});
{% endhighlight %}
{% highlight C++ %}
leftClick.Emit();  // output: clicked
rightClick.Emit(); // output: clicked
{% endhighlight %}
`Merge` takes a variable number of arguments, so more than two streams can be merged at once.

An alternative is using the overloaded `|` operator for merging:
{% highlight C++ %}
EventsT<> anyClick = leftClick | rightClick;
{% endhighlight %}


## Processing events

Besides merging events from multipe streams, we can also process the events themselves.

First, let's demonstrate this by filtering a stream of numbers:
{% highlight C++ %}
EventSourceT<int> numbers = MakeEventSource<D,int>();

EventsT<int> greater10 = Filter(numbers, [] (int n) {
    return n > 10;
});
{% endhighlight %}
{% highlight C++ %}
Observe(greater10, [] (int n) {
    cout << n << endl;
});
{% endhighlight %}
{% highlight C++ %}
numbers << 5 << 11 << 7 << 100; // output: 11, 100
{% endhighlight %}
If the filter predicate function returns true for the passed value, it will be forwarded. Otherwise, it's filtered out.

Events can also be transformed with `Transform`. In other functional programming languages this operation would be called map, but we stick to the naming of `std::transform`.
For example, we can transform a stream of numbers into a `std::pair` of the number and a tag that indicates whether the former exceeded a certain threshold:
{% highlight C++ %}
enum Tag { normal, critical };

using TaggedNum = pair<Tag,int>;

EventSourceT<int>  numbers = MakeEventSource<D,int>();
EventsT<TaggedNum> tagged  = Transform(numbers, [] (int n) {
    if (n > 10)
        return TaggedNum( critical, n );
    else
        return TaggedNum( normal, n );
});
{% endhighlight %}
{% highlight C++ %}
Observe(tagged, [] (const TaggedNum& t) {
    if (t.first == critical)
        cout << "(critical) " << t.second << endl;
    else
        cout << "(normal)  " << t.second << endl;
});
{% endhighlight %}
{% highlight C++ %}
numbers << 5;
// output: (normal) 5

numbers << 20; 
// output: (critical) 20
{% endhighlight %}


## Changing multiple inputs

Queing multiple inputs in a single turn works analogously to signals:
{% highlight C++ %}
DoTransaction<D>([] {
    src << 1 << 2 << 3;
    src << 4;
});
{% endhighlight %}
It's possible to mix signal and event input in the same transaction.

Unlike signals, where only the last value change for each signal is used, event streams will forward all queued values:
{% highlight C++ %}
EventSourceT<int> src = MakeEventSource<D,int>();

Observe(src, [] (int v) {
    cout << v << endl;
});
// Turn #1, output: 1, 2, 3, 4
{% endhighlight %}