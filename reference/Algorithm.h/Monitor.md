---
layout: default
title: Monitor
type_tag: function
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Algorithm.h, url: 'reference/Algorithm.h/'}
---

Emits value changes of target signal as events.

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