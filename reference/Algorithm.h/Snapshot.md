---
layout: default
title: Snapshot
type_tag: function
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Algorithm.h, url: 'reference/Algorithm.h/'}
---

Sets the signal value to the value of a target signal when an event is received.

## Syntax
{% highlight C++ %}
template
<
    typename D,
    typename S,
    typename E
>
Signal<D,S> Snapshot(const Events<D,E>& trigger, const Signal<D,S>& target);
{% endhighlight %}

## Semantics
Creates a signal with value `v = target.Value()`.
The value is set on construction and updated **only** when receiving an event from `trigger`.

## Graph
<img src="{{ site.baseurl }}/media/flow_snapshot.png" alt="Drawing" width="500px"/>

