---
layout: default
title: MakeContinuation
type_tag: function
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Domain.h, url: 'reference/Domain.h/'}
---
## Syntax
{% highlight C++ %}
// (1)
template
<
    typename D,
    typename D2 = D,
    typename S,
    typename FIn
>
Continuation<D,D2> MakeContinuation(const Signal<D,S>& trigger, FIn&& func);

// (2)
template
<
    typename D,
    typename D2 = D,
    typename E,
    typename FIn
>
Continuation<D,D2> MakeContinuation(const Events<D,E>& trigger, FIn&& func);

// (3)
template
<
    typename D,
    typename D2 = D,
    typename E,
    typename FIn,
    typename ... TDepValues
>
Continuation<D,D2>
    MakeContinuation(const Events<D,E>& trigger,
                     const SignalPack<D,TDepValues...>& depPack, FIn&& func);
{% endhighlight %}

## Semantics
(1) When the signal value `s` of `trigger` changes, `func(s)` is executed in a transaction of domain `D2`.
In pseudo code:
{% highlight C++ %}
DoTransaction<D2>([func, s] {
    func(s)
});
{% endhighlight %}

(2) For every event `e` in `trigger`, `func(e)` is called in a transaction of domain `D2`.
Multiple events from the same turn are captured in a single transaction.
In pseudo code:
{% highlight C++ %}
DoTransaction<D2>([func, events] {
    for (const auto& e : events)
        func(e);
});
{% endhighlight %}

(3) Similar to (2), but the synchronized values of signals in `depPack` are passed to `func` as additional arguments.
Changes of signals in `depPack` do not trigger an update - only received events do.
In pseudo code:
{% highlight C++ %}
DoTransaction<D2>([func, events, depValues...] {
    for (const auto& e : events)
        func(e, depValues ...);
});
{% endhighlight %}

The signature of `func` should be equivalent to:

* (1) `void func(const S&)`
* (2) `void func(const E&)`
* (3) `void func(const E&, const TDepValues& ...)`

{% highlight C++ %}
DoTransaction<D2>([func, events, depValues...] {
    for (const auto& e : events)
        func(e, depValues ...);
});
{% endhighlight %}