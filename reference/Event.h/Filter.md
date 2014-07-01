---
layout: default
---
# Filter

## Syntax
{% highlight C++ %}
// (1)
template
<
    typename D,
    typename E,
    typename F
>
TempEvents<D,E,/*unspecified*/>
    Filter(const Events<D,E>& source, F&& func);

// (2)
template
<
    typename D,
    typename E,
    typename F
>
TempEvents<D,E,/*unspecified*/>
    Filter(TempEvents<D,E,/*unspecified*/>&& source, F&& func);

// (3)
template
<
    typename D,
    typename E,
    typename ... TDepValues,
    typename F
>
Events<D,E> Filter(const Events<D,E,/*unspecified*/>& source,
                   const SignalPack<D,TDepValues...>& depPack, F&& func);
{% endhighlight %}

## Semantics
For every event `e` in `source`, emit `e` if `func(e) == true`.

(1) Takes an l-value const reference.

(2) Takes an r-value reference. The linked node is combined with the new node.

(3) Similar to (1), but the synchronized values of signals in `depPack` are passed to `func` as additional arguments.

**Note**: Changes of signals in `depPack` do not trigger an update - only received events do.

The signature of `func` should be equivalent to:

* (1,2) `bool func(const E&)`
* ( 3 ) `bool func(const E&, const TDepValues& ...)`

## Graph
(1) <br/>
<img src="{{ site.baseurl }}/media/flow_filter.png" alt="Drawing" width="500px"/>

(3) <br/>
<img src="{{ site.baseurl }}/media/flow_filter2.png" alt="Drawing" width="500px"/>