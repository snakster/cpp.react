---
layout: default
title: Concurrency model
groups: 
 - {name: Home, url: ''}
 - {name: Guides , url: 'guides/'}
---

* [Goals](#goals)
* [Parallel updating](#parallel-updating)
* [Concurrent input](#concurrent-input)
* [Conclusion](#conclusion)

## Goals

The goal of C++React is to provide lightweight implicit parallelism for a shared memory model.

TODO


## Parallel updating

The [Dataflow model guide](Dataflow-model.html) showed how interdependent reactive values can be represented as graphs.
It further showed how inputs are grouped into transactions and processed in turns that propagate changes through the graph.

Based on this dataflow model, we can immediately devise a scheme to parallelize the updating process:
When there are more than two outgoing edges, both paths can be traversed in parallel by different threads.
Coupled with the requirement for update minimality, if a node has multiple predecessors, only the last arriving thread coming from a predecessor may proceed.

In other words, threads are forked and joined analogously to the paths in the graph.
In our implementation, we do not use threads for parallelization, but rather tasks, which are automatically mapped to a thread pool.
The following figure shows an example:

<img src="{{ site.baseurl }}/media/conc1.png" alt="Drawing" width="500px"/>

Conceptionally, propagation is inherently parallelizable and synchronization can be implemented efficiently based on atomic counters.
Concrete algorithms may be slightly more complex, in particular because they have to account for topology changes during updating due to dynamic nodes.


### Trivial node updates

One particular challenge is parallelizing graphs that consist of mostly trivial operations, because tasks require a certain workload to outweight the induced overhead.
Thus, processing such trivial nodes in a dedicated task is not worth it.
The different propagation strategies account for that with different methods,
for example by dynamically agglomerating several trivial nodes into a single task, or by selectively enabling parallel updating for heavyweight nodes only.

The problem is especially relevant when using overloaded operators to lift reactive expressions.
An expression like `SignalT<int> x = a * 10 + b * 20 - c`, with `a,b,c` being signals as well, will result in 4 nodes - one for each binary operand.
To reduce the overhead, a template-based technique to merge r-value expressions is applied.
At the cost of a small overhead at creation time, the presented expression only results in a single node.

Yet parallelizing trivial updates remains difficult as the overall implementation becomes more optimized.
That is because by removing overhead, the workload per task is reduced as well.


## Concurrent input

While parallelization of updates in the same turn comes natural, there is rather substantial downside:
All reactive values of a domain are eventually "connected"; even if seemingly unrelated input nodes are changed, their propagation paths might cross at some point.

In worst case, this means that each domain can only process a single transaction at a time.
There are multiple ways in which it could be addressed; the first is to drop update minimality and glitch freedom.
Coordinated updating becomes synchronized updating/flooding.

Another approach would be to divide the graph into disjoint regions, either based on static topology, or by having turns acquire locks on the nodes they might update beforehand.
The following image visualizes this:

<img src="{{ site.baseurl }}/media/conc2.png" alt="Drawing" width="500px"/>

The problem, however, are dynamic nodes. The nature of dynamic nodes is that they change the topology as part of being updated; we do know in which way until after the update.
Thus, in the presence of dynamic node, these regions may be extended dynamically.
This can lead to scenarios where several turns have already started and cannot continue unless they start rolling back already committed updates.

While a rollback of updates would not be impossible, for this library it was considered as too heavyweight.
Since giving up update minimality and glitch freedom was not desirable either, we use a different approach.


### Transaction merging

So far, two dimensions of parallelization have been defined:

* horizontal: running updates of the same turn in parallel;
* vertical: running multiple turns concurrently.

Supporting the latter is difficult, but we can indirectly enable it by merging multiple transactions into a single turn.
Here's how that works:

* Normally, transactions are processed in a dedicated turn.
* Each turn has exclusive access to the whole graph.
* If a transaction arrives while another turn is active, the transaction is queued.
* Successive transactions in the queue that are mergeable will be merged. Their inputs are applied in FIFO order.
* Merged transactions are processed in a single turn.

Each turn still has exclusive access to the whole graph, but the workload per turn is increased.
Instead of adding another dimension, we attempt to extend the existing one:

<img src="{{ site.baseurl }}/media/conc3.png" alt="Drawing" width="500px"/>

* More changed input nodes per turn -> more nodes of the graph are reached -> more opportunity for horizontal parallelization.
* More workload per node -> less significant overhead, both in terms of processing and parallelization.
* Less turns -> less overhead.

The benefits w.r.t. to reduced overhead do always apply, but to gain increased horizontal parallelization, the graph topology must allow for it.

Merging happens completely transparent, and it may cross boundaries between asynchronous and synchronous transactions.
There is one particular caveat: For signals, only the last value change during a transaction is applied.
The reasoning behind this is that unlike for event streams, where every single event should be registered,
for signals we only care about the most recent value.
If a value is already known to be outdated, there is no need to process it.
This rule is extended to merged transactions, which only apply the most recent value for each changed input signal.

There might also be cases where certain invariants exist between inputs of the same transaction, which would be violated if transactions are merged.
For these reasons, transaction merging is not enabled by default, but has to be allowed explicitly.