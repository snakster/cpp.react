---
layout: default
---
## Syntax
{% highlight C++ %}
// (1)
template
<
    typename D,
    typename E,
    typename V,
    typename F,
    typename S = decay<V>::type
>
Signal<D,S> Iterate(const Events<D,E>& events, V&& init, F&& func); 

// (2)
template
<
    typename D,
    typename E,
    typename V,
    typename F,
    typename ... TDepValues,
    typename S = decay<V>::type
>
Signal<D,S> Iterate(const Events<D,E>& events, V&& init,
                    const SignalPack<D,TDepValues...>& depPack, F&& func); 
{% endhighlight %}

## Semantics
(1) Creates a signal with an initial value `v = init`.

- If the return type of `func` is `S`: For every received event `e` in `events`, `v` is updated to `v = func(e,v)`.

- If the return type of `func` is `void`: For every received event `e` in `events`, `v` is passed by non-cost reference to `func(e,v)`, making it mutable.
  This variant can be used if copying and comparing `S` is prohibitively expensive.
  Because the old and new values cannot be compared, updates will always trigger a change.

(2) Similar to (1), but the synchronized values of signals in `depPack` are passed to `func` as additional arguments. Changes of signals in `depPack` do _not_ trigger an update - only received events do.

The signature of `func` should be equivalent to:

* (1a) `S func(const E&, const S&)`
* (1b) `void func(const E&, S&)`
* (2a) `S func(const E&, const S&, const TDepValues& ...)`
* (2b) `void func(const E&, S&, const TDepValues& ...)`

## Graph
(1a) <br/>
<img src="{{ site.baseurl }}/media/flow_iterate.png" width="500px" />

(2a) <br/>
<img src="{{ site.baseurl }}/media/flow_iterate2.png" width="500px" />
