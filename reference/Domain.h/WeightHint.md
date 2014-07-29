---
layout: default
title: WeightHint
type_tag: enum class
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Domain.h, url: 'reference/Domain.h/'}
---
This is the parameter type of the `SetWeightHint` member function all reactive types (i.e. Signal, Events, Observer, ...) have in common.
It can be used to hint to the propagation engine whether parallelizing the update of the respective linked node is worthwhile.
The engine will decide if and how to act on the hint.

## Definition

{% highlight C++ %}
enum class WeightHint
{
    automatic,
    light,
    heavy
};
{% endhighlight %}

## Semantics

* `automatic`: Whether a node is heavyweight decided automatically. This is the default setting for all nodes.
  When it makes sense, this usually means that the execution time of an update is measured and compared to a threshold at runtime.
  Whenever `automatic` is set as a hint, the weight state will be reset.

* `light`: The node is lightweight; its update duration will not be measured at runtime.
  The propagation engine will usually try to execute multiple lightweight node updates in a single parallel task.

* `heavy`: The node is heavyweight; its update duration will not be measured at runtime.
  The propagation engine will usually try to spawn a dedicated parallel task for each heavyweight node.