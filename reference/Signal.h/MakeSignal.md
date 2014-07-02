---
layout: default
title: MakeSignal
type_tag: function
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Signal.h, url: 'reference/Signal.h/'}
---
## Syntax
{% highlight C++ %}
template
<
    typename D,
    typename F,
    typename ... TValues,
    typename S = result_of<F(TValues...)>::type
>
TempSignal<D,S,/*unspecified*/>
    MakeSignal(F&& func, const Signal<D,TValues>& ... args);
{% endhighlight %}

## Semantics
Creates a new signal node with value `v = func(args.Value(), ...)`.
This value is set on construction and updated when any `args` have changed.

The signature of `func` should be equivalent to:

* `S func(const TValues& ...)`

## Graph
<img src="{{ site.baseurl }}/media/flow_makesignal.png" alt="Drawing" width="500px"/>