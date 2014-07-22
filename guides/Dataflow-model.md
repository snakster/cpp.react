---
layout: default
title: Dataflow model
groups: 
 - {name: Home, url: ''}
 - {name: Guides , url: 'guides/'}
---

* [Graph model](#graph-model)
* [Propagation model](#propagation-model)


## Graph model

[Introduction to C++React]() presented several types of reactive values to model dataflow declarativly.

A pratical analogy is to think of this as a system of assembly lines that process arbitrary items.
We declare the paths on which items should move through a series of work stations.
Once we did that, the system is self-operational and the logistic details are handled automatically.

A more formal method to model the dataflow are directed acyclic graphs (DAGs), which can be constructed from the dependency relations between reactive values.
Each entity is a node and directed edges denote data propagation paths.

To give an example, let `a`, `b` and `c` be arbitrary signals.
`x` is another signal that is calculated based on the former.
Instead of invoking `MakeSignal` explicitly, the overloaded `+` operator is used to achieve the same result.
{% highlight C++ %}
SignalT<S> x = (a + b) + c;
{% endhighlight %}
This is the matching dataflow graph:
<br />
<img src="{{ site.baseurl }}/media/signals1.png" alt="Drawing" width="500px"/>

A similar example could've been given for event streams.
From a dataflow perspective, what kind of data is propagated and what exactly happens to it in each node are not relevant.

C++React does not expose the graph data structures directly to the user; instead, they are wrapped by lightweight proxies.
Such a proxy is essentially a shared pointer to the heap-allocated node.
Examples of proxy types are `Signal`, `Events`, `Observer`.
The concrete type of the node is hidden behind the proxy.

We show this scheme for the previous example:
{% highlight C++ %}
SignalT<S> a = MakeVar(...);
SignalT<S> b = MakeVar(...);
SignalT<S> c = MakeVar(...);
SignalT<S> x = (a + b) + c;
{% endhighlight %}

The `MakeVar` function allocates the respective node and linking it to the returned proxy.

One observation made from the previous example is that not all nodes in the graph are bound to a proxy; the temporary sub-expression `a + b` results in a node as well.
If a new node is created, it takes shared ownership of its dependencies, because it needs them to calculate its own value. This prevents the `a + b` node from disappearing.

The resulting reference graph is similar to the dataflow graph, but with reverse edges (and as such, a DAG as well): <br />
<img src="{{ site.baseurl }}/media/signals2.png" alt="Drawing" width="500px"/>

The number inside each node denotes its reference count. On the left are the proxy instances exposed by the API.
Assuming the handles for `a`, `b` and `c` would go out of scope but `x` remains, the reference count of all nodes is still 1, until `x` disappears as well.
Once that happens, the graph is deconstructed from the bottom up.


### Input and output nodes

From now on, we refer to the set of inter-connected reactive values as a reactive system.
A closed, self-contained reactive system would ultimately be useless, as there's no way to get information in or out.
In other words, mechanisms are required to

* react to external input; and
* propagate side effects to the outside.

The outside refers to the larger context of the program the reactive system is part of.

To address the first requirement, there exist designated `input nodes` at the root of the graph.
We've already seen examples of those in the form of `VarSignal` and `EventSource`.
They are the input interface of the reactive system and can be manipulated imperatively.
This allows integration of a reactive system with an imperative program.

Propagating changes to the outside world could happen at any place through side effects, since C++ does not provide any means to enforce functional purity.
However, since side effects have certain implications on thread-safety and our ability to reason about program behaviour, by convention they're moved them to designated `output nodes`.
By definition, these nodes don't have any successors. Analogously to input nodes, they are the output interface of the reactive system.
In [Introduction to C++React]() we've already introduced examples of output nodes in the form of observers.


### Domains

Organizing all reactive values in a single graph would become increasingly difficult to manage.
For this reason, we allow multiple graphs in the form of domains.
Each domain is independent and groups related reactives.
The implementation uses static type tags, so the compiler prevents combination of reactives from different domains at compile time.
The downside is that the domain tag becomes part of the type, so
`Signal<S>` becomes `Signal<D,S>`, where `D` is the domain name.
To reduce the amount of typing, there exists a macro to define scoped aliases for a given

Domains can communicate with each other by sending asynchrounous messages from special output nodes called continuations to input nodes of other domains, including themselves.
The following figure outlines this model:<br />
<img src="{{ site.baseurl }}/media/domain1.png" alt="Drawing" width="500px"/>


### Cycles

When creating a reactive value, all its dependencies have to be passed upon initialization.
For this reason, graphs are acyclic by definition.
There is one exception: Certain types of nodes - we refer to them as dynamic - can change their dependencies after initialization.
Creating cyclic graphs this way results in undefined behaviour.

For inter-domain communcation, cyclic dependencies between domains are allowed.
This means that two domains could bounce messages off each other infinitely.
It's up to the programmer to ensure that such loops terminate eventually.

## Propagation model



## Conclusion

In summary:

* Intra-domain dependency relations are formulated declaratively and structured as a DAG. Communication is handled implicitly and provides certain guarantees w.r.t. to ordering.
* Inter-domain communciation uses asychronous messaging. Messages are dispatched imperatively without any constraints.