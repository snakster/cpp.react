---
layout: default
title: Join
type_tag: function
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Event.h, url: 'reference/Event.h/'}
---
## Syntax
{% highlight C++ %}
template
<
    typename D,
    typename ... TValues
>
Events<D,std::tuple<TValues...>>
    Join(const Events<D,TValues>& ... sources);
{% endhighlight %}

## Semantics
Emit a tuple (e1,...,eN) for each complete set of values for sources 1 ... N.

Each source slot has its own unbounded buffer queue that persistently stores incoming events.
For as long as all queues are not empty, one value is popped from each and emitted together as a tuple.

## Graph
<img src="{{ site.baseurl }}/media/flow_join.png" alt="Drawing" width="500px"/>