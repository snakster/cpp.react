---
layout: default
---
# MakeVar

## Syntax
{% highlight C++ %}
template
<
    typename D,
    typename V,
    typename S = decay<V>::type,
>
VarSignal<D,S> MakeVar(V&& init);
{% endhighlight %}

## Semantics
Creates a new input signal node and links it to the returned `VarSignal` instance.

## Graph
<img src="{{ site.baseurl }}/media/flow_makevar.png" alt="Drawing" width="500px"/>