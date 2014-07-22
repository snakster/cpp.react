---
layout: default
title: Pulse
type_tag: function
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Algorithm.h, url: 'reference/Algorithm.h/'}
---

Emits the value of a target signal when an event is received.

## Syntax
{% highlight C++ %}
template
<
    typename D,
    typename S,
    typename E
>
Events<D,S> Pulse(const Events<D,E>& trigger, const Signal<D,S>& target);
{% endhighlight %}

## Semantics
Creates an event stream that emits `target.Value()` when receiving an event from `trigger`
The values of the received events are irrelevant.

## Graph
<img src="{{ site.baseurl }}/media//flow_pulse.png" alt="Drawing" width="500px"/>