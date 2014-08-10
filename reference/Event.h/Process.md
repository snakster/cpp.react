---
layout: default
title: Process
type_tag: function
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Event.h, url: 'reference/Event.h/'}
---
## Syntax
{% highlight C++ %}
// (1)
template
<
    typename T,
    typename D,
    typename E,
    typename F
>
Events<D,T> Process(const Events<D,E>& source, F&& func);

// (2)
template
<
    typename T,
    typename D,
    typename E,
    typename F,
    typename ... TDepValues
>
Events<D,T> Process(const Events<D,E>& source,
                    const SignalPack<D,TDepValues...>& depPack, F&& func);
{% endhighlight %}

## Semantics
(1) `func(range, out)` is called with all events `range` from source in current turn, under the condition that the number of events is greater zero.
New events are emitted through `*out = e`.

(2) Similar to (1), but the synchronized values of signals in `depPack` are passed to `func` as additional arguments.

**Note**: Changes of signals in `depPack` do not trigger an update - only received events do.

The signature of `func` should be equivalent to:

* (1) `void func(EventRange<E> range, EventEmitter<T> out)`
* (2) `void func(EventRange<E> range, EventEmitter<T> out, const TDepValues& ...)`

The type of outgoing events `T` has to be specified explicitly, i.e. `Process<T>(src, func)`.

`EventRange` is an adapter that exposes random access input iterators to begin and end of the source event container.
`EventEmitter` is a `std::back_insert_iterator` of the output event container.

## Graph
(1) <br/>
<img src="{{ site.baseurl }}/media/flow_process.png" alt="Drawing" width="500px"/>

(2) <br/>
<img src="{{ site.baseurl }}/media/flow_process2.png" alt="Drawing" width="500px"/>