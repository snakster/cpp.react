---
layout: default
---
# Flatten

## Syntax
{% highlight C++ %}
template
<
    typename D,
    typename T
>
Signal<D,T> Flatten(const Signal<D,Signal<D,T>>& other);
{% endhighlight %}

## Semantics
TODO

## Graph
<img src="{{ site.baseurl }}/media/flow_flatten.png" alt="Drawing" width="500px"/>