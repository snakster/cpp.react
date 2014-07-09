---
layout: default
title: Graph model
groups: 
 - {name: Home, url: ''}
 - {name: Guides , url: 'guides/'}
---

Reactive programming is a declarative paradigm.
The programmer states _how_ to calculate something, for example in terms of a function, but the actual execution of these calculations is implicitly scheduled by a runtime engine.

From the dependency relations between reactive values, we can construct a graph that models the dataflow.
Each reactive value is a node and directed edges denote data propagation paths.

To give an example, let `a`, `b` and `c` be arbitrary reactive values.
`x` is another reactive value that combines the former through use a binary operation `+`, which can be applied to reactive operands:
{% highlight C++ %}
x = (a + b) + c;
{% endhighlight %}
This is the matching dataflow graph:
<br />
<img src="{{ site.baseurl }}/media/signals1.png" alt="Drawing" width="500px"/>

C++React does not expose the graph data structures directly to the user; instead, they are wrapped by lightweight proxies.
Such a proxy is essentially a shared pointer to the heap-allocated node.
The concrete type of the node is hidden behind the proxy.

We show this scheme with an arbirary proxy type `R`:
{% highlight C++ %}
R a = MakeR(...);
R b = MakeR(...);
R c = MakeR(...);

R x = (a + b) + c;
{% endhighlight %}

The `MakeR` function is responsible for allocating the respective node and linking it to the returned proxy.

One observation made from the previous example is that not all nodes in the graph are bound to a proxy; the temporary sub-expression `a + b` results in a node as well.
If a new node is created, it takes shared ownership of its dependencies, because it needs them to calculate its own value. This prevents the `a + b` node from disappearing.

The resulting reference graph is similar to the dataflow graph, but with reverse edges (and as such, a DAG as well): <br />
<img src="{{ site.baseurl }}/media/signals2.png" alt="Drawing" width="500px"/>

The number inside each node denotes its reference count. On the left are the proxy instances exposed by the API.
Assuming the handles for `a`, `b` and `c` would go out of scope but `x` remains, the reference count of all nodes is still 1, until `x` disappears as well.
Once that happens, the graph is deconstructed from the bottom up.


## Input and output nodes

A closed, self-contained reactive system would ultimately be useless, as there's no way to get information in or out.
In other words, a reactive system needs to be able to

* react to external events; and
* propagate side effects to the outside.

The outside refers to the larger context of the program the reactive system is part of.

To address the first requirement, we introduce designated `input nodes` at the root of the graph.
They are the input interface of the reactive system and can be manipulated imperatively.
This allows integration of a reactive system with an imperative program.

Propagating changes to the outside world could happen at any place, since C++ does not provide any means to enforce functional purity.
However, since side effects have certain implications on thread-safety and our ability to reason about program behaviour, by convention we move them to designated `output nodes`.
By definition, these nodes don't have any successors. Analogously to input nodes, they are the output interface of the reactive system.


## Domains

Organizing all reactive values in a single graph would become increasingly difficult to manage.
For this reason, we allow multiple graphs in the form of domains.
Each domain is independent and groups related reactives.
The implementation uses static type tags, so the compiler prevents combination of reactives from different domains at compile time.

Domains can communicate with each other through their regular input and output interfaces, i.e. they can send messages from their output nodes to input nodes of other domains, including themselves.
The following figure outlines this model:<br />
<img src="{{ site.baseurl }}/media/domain1.png" alt="Drawing" width="500px"/>

In summary:

* Intra-domain dependency relations are formulated declaratively and structured as a DAG. Communication is handled implicitly and provides certain guarantees w.r.t. to ordering etc.
* Inter-domain communciation uses asychronous messaging. Messages are dispatched imperatively without any constraints.


## Cycles

When creating a reactive value, all its dependencies have to be passed upon initialization.
For this reason, graphs are acyclic by definition.
There is one exception: Certain types of nodes - we refer to them as dynamic - can change their dependencies after initialization.
Creating cyclic graphs this way results in undefined behaviour.

For inter-domain communcation, cyclic dependencies between domains are allowed.
This means that two domains could bounce messages off each other infinitely.
It's up to the programmer to ensure that such loops terminate eventually.