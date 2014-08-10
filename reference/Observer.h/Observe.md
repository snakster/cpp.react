---
layout: default
title: Observe
type_tag: function
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Observer.h, url: 'reference/Observer.h/'}
---
## Syntax
{% highlight C++ %}
// (1)
template
<
    typename D,
    typename S,
    typename F
>
Observer<D> Observe(const Signal<D,S>& subject, F&& func);

// (2)
template
<
    typename D,
    typename E,
    typename F
>
Observer<D> Observe(const Events<D,E>& subject, F&& func);

// (3)
template
<
    typename D,
    typename F,
    typename E,
    typename ... TDepValues
>
Observer<D> Observe(const Events<D,E>& subject,
                    const SignalPack<D,TDepValues...>& depPack, F&& func);
{% endhighlight %}

## Semantics
(1) When the signal value `s` of `subject` changes, `func(s)` is called.

(2) For every event `e` in `subject`, `func(e)` is called.

(3) Similar to (2), but the synchronized values of signals in `depPack` are passed to `func` as additional arguments. Changes of signals in `depPack` do not trigger an update - only received events do.

The signature of `func` should be equivalent to:

* (1) `TRet func(const S&)`
* (2) `TRet func(const E&)`
* (3) `TRet func(const E&, const TDepValues& ...)`

`TRet` can be either `ObserverAction` or `void`.
The event parameter `const E&` can also be replaced by an event range, i.e. `TRet func(EventRange<E> range, const TDepValues& ...)` for case (3).
This allows for explicit batch processing of events of a single turn.



By returning `ObserverAction::stop_and_detach`, the observer function can request its own detachment.
Returning `ObserverAction::next` keeps the observer attached. Using a `void` return type is the same as
always returning `ObserverAction::next`.

## Graph
(1) <br/>
<img src="{{ site.baseurl }}/media/flow_observe2.png" alt="Drawing" width="500px"/>

(2) <br/>
<img src="{{ site.baseurl }}/media/flow_observe.png" alt="Drawing" width="500px"/>

(3) <br/>
<img src="{{ site.baseurl }}/media/flow_observe3.png" alt="Drawing" width="500px"/>