---
layout: default
title: Enabling parallelization
groups: 
 - {name: Home, url: ''}
 - {name: Tutorials , url: 'tutorials/'}
---

- [Parallel updating](#parallel-updating)
- [Concurrent input](#concurrent-input)
- [Selecting a propagation engine](#selecting-a-propagation-engine)

## Parallel updating

Enabling parallel propagation of changes - or parallel updating as we refer to it from now on - turns out to be straightforward;
all we have to do is selecting the `parallel` policy in the domain definition:
{% highlight C++ %}
#include "react/Domain.h"
#include "react/Signal.h"
#include "react/Event.h"

REACTIVE_DOMAIN(D, parallel)
USING_REACTIVE_DOMAIN(D)

int calcX(int);
int calcY(int);
int calcZ(int); 

VarSignalT<int> Input = MakeVar<D>(0);
SignalT<int>    X     = MakeSignal(Input, calcX);
SignalT<int>    Y     = MakeSignal(Input, calcY);
SignalT<int>    Z     = MakeSignal(With(X, Y), calcZ);
{% endhighlight %}

Assuming `calcX` and `calcY` are computationally expensive, they could be re-calculated in parallel after `Input` has been changed.
How exactly this is handled internally is up to the propagation engine; it's explained in detail [here].
In summary, the propagation engine will

- use TBB tasks to parallelize upates, which in turn are mapped to a thread pool;
- try to identify expensive computations at runtime to determine when parallelization is worthwhile;
- agglomerate computations dynamically to reduce overhead.


## Concurrent input

Parallel updating is _internal_ concurrency, as it may execute multiple updates of the same turn concurrently.
But what we really want there is parallelism - that is, running them on multiple CPUs in parallel.

On the other hand, there's _external_ concurrency, which is refers to the ability of handling concurrent input.
This is similar to how a concurrent queue supports push and pop operations from multiple threads at the same time.

To enable concurrent input, we use a concurrency policy with the `_concurrent` suffix:
{% highlight C++ %}
REACTIVE_DOMAIN(D1, sequential_concurrent)
REACTIVE_DOMAIN(D2, parallel_concurrent)

D1::EventSourceT<int> Source1 = MakeEventSource<D1,int>();
D2::EventSourceT<int> Source2 = MakeEventSource<D2,int>();
{% endhighlight %}

{% highlight C++ %}
auto f = [&] {
    for (int i=0; i<K; i++)
    {
        Source1 << i;
        Source2 << i;
    }
};

std::thread t1(f);
std::thread t2(f);
{% endhighlight %}

Both domains `D1` and `D2` now support safe concurrent input, which is independent of whether updates are parallelized or not.


## Selecting a propagation engine

C++React supports multiple propagation engines which implement different propagation strategies.
So far, we only selected the concurrency policy when defining a domain.
This uses a default engine for that particular policy.

Selecting an engine explicitly is mostly just relevant for parallel updating, as there's just a single engine that supports the `sequential` policy.

For `parallel` (and `parallel_concurrent`), we have the following options:
{% highlight C++ %}
REACTIVE_DOMAIN(D1, parallel, ToposortEngine)
REACTIVE_DOMAIN(D2, parallel, PulsecountEngine)
REACTIVE_DOMAIN(D3, parallel, SubtreeEngine)
{% endhighlight %}

How exactly each strategy works is explained [here]({{ site.baseurl }}/guides/engines/).
Since exchanging them is trivial, it's easy to compare their performance and find the one that works best for a particular scenario.