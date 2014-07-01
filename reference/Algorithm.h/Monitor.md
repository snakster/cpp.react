---
layout: default
---
# Monitor

## Syntax
{% highlight C++ %}
template
<
    typename D,
    typename S
>
Events<D,S> Monitor(const Signal<D,S>& target);
{% endhighlight %}

## Semantics
When `target` changes, emit the new value `e = target.Value()`.

## Graph
<img src="{{ site.baseurl }}/media/flow_monitor.png" alt="Drawing" width="500px"/>