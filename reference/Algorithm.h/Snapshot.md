---
layout: default
---
## Snapshot
###### Syntax
{% highlight C++ %}
template
<
    typename D,
    typename S,
    typename E
>
Signal<D,S> Snapshot(const Events<D,E>& trigger, const Signal<D,S>& target);
{% endhighlight %}

###### Graph
<img src="{{ site.url }}/images/flow_snapshot.png" alt="Drawing" width="500px"/>

###### Semantics
Creates a signal with value `v = target.Value()`.
The value is set on construction and updated **only** when receiving an event from `trigger`.