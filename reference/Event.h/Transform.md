---
layout: default
title: Transform
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
    typename D,
    typename E,
    typename F,
    typename T = result_of<F(E)>::type
>
TempEvents<D,T,/*unspecified*/>
    Transform(const Events<D,E>& source, F&& func);

// (2)
template
<
    typename D,
    typename E,
    typename F,
    typename T = result_of<F(E)>::type
>
TempEvents<D,T,/*unspecified*/>
    Transform(TempEvents<D,E,/*unspecified*/>&& source, F&& func);

// (3)
template
<
    typename D,
    typename E,
    typename F,
    typename ... TDepValues,
    typename T = result_of<F(E,TDepValues...)>::type
>
Events<D,T> Transform(const Events<D,E>& source,
                      const SignalPack<D,TDepValues...>& depPack, F&& func);
{% endhighlight %}

## Semantics
For every event `e` in `source`, emit `t = func(e)`.

(1) Takes an l-value const reference.

(2) Takes an r-value reference. The linked node is combined with the new node.

(3) Similar to (1), but the synchronized values of signals in `depPack` are passed to `func` as additional arguments.

**Note**: Changes of signals in `depPack` do not trigger an update - only received events do.

The signature of `func` should be equivalent to:

* (1,2) `T func(const E&)`
* ( 3 ) `T func(const E&, const TDepValues& ...)`

## Graph
(1) <br/>
<img src="{{ site.baseurl }}/media/flow_transform.png" alt="Drawing" width="500px"/>

(3) <br/>
<img src="{{ site.baseurl }}/media/flow_transform2.png" alt="Drawing" width="500px"/>