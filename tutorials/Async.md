---
layout: default
title: Asynchronous transactions and continuations
groups: 
 - {name: Home, url: ''}
 - {name: Tutorials , url: 'tutorials/'}
---

- [Asynchronous transactions](#asynchronous-transactions)
- [Continuations](#continuations)

## Asynchronous transactions

Earlier tutorials showed the use of `DoTransaction`, which applies multiple inputs as a transaction.
`DoTransaction` is synchronous, so it the propagation runs in the current thread.
For concurrent input, this means that multiple threads have to synchronize, which includes potential waiting if the propagation engine is busy.

If the calling thread has to return immediately, for example to ensure responsiveness, `AsyncTransaction` exists an asynchrounous, non-blocking alternative to `DoTransaction`.
It follows a producer/consumer scheme by enqueuing the transaction function for another dedicated worker thread.

Here's an example:
{% highlight C++ %}
#include "react/Domain.h"
#include "react/Event.h"
#include "react/Observer.h"

REACTIVE_DOMAIN(D, sequential_concurrent)
USING_REACTIVE_DOMAIN(D)

EventSourceT<int>   Samples     = MakeEventSource<D,int>();
{% endhighlight %}
{% highlight C++ %}
Observe(Samples, [] (int v) {
    cout << v << endl;
});

TransactionStatus status;

AsyncTransaction<D>(status, [&] {
    Samples << 30 << 31 << 31 << 32;
});

AsyncTransaction<D>(status, [&] {
    Samples << 40 << 41 << 51 << 62;
});

// Do other things...

// Block the current thread until both transactions have been processed
status.Wait();
{% endhighlight %}

One thing that should be noted here is that concurrent input is not the same as concurrent execution; the transactions in this example will not be interleaved and they will be processed in the order they've been queued in.

The `TransactionStatus` instance provides a handle that allows waiting for an asynchronous transaction.
The same status can be used for multiple transactions, and it can be re-used after `Wait()`.

## Continuations

Reactives from different domains cannot be combined - the static type checking of the domain tag prevents it. This, for example, is not possible:

{% highlight C++ %}
REACTIVE_DOMAIN(D1, sequential_concurrent)
REACTIVE_DOMAIN(D2, sequential_concurrent)
REACTIVE_DOMAIN(D3, sequential_concurrent)

D1::SignalT<int>   A = ...;
D2::SignalT<int>   B = ...;

D3::SignalT<int>   C = A + B; // Domain mismatch between D1 and D2.
D3::SignalT<int>   D = A + A; // Domain mismatch between D1 and D3.
{% endhighlight %}

There is, however, nothing that prevents us from using imperative input enable inter-domain communication. For example:

{% highlight C++ %}
// Note: Do not actually do this, it will deadlock.
D1::VarSignalT<int>   A = ...;
D2::VarSignalT<int>   B = ...;

Observe(A, [&] (int v) {
	B.Set(v);
})

// Or how about this one?
Observe(A, [&] (int v) {
	A.Set(v);
})
{% endhighlight %}

While this is possible, it is very easy to create situations where two transactions block each other and neither of them can make progress.
The reason for this is that the new transaction is started from inside the observer, thus it may block until the called transaction completes.
If the callee runs on the same domain as the caller - as it is the case for the latter example, where a domain targets its own input node - this would result in a deadlock.

To avoid this, there exists a special reactive type `Continuation` to bridge between two domains (from source to target).
Continuations are similar to observers, in the sense that they allow to attach callback functions to other reactive values.
The difference is that these functions will be treated as if they had been passed to `AsyncTransaction` of the target domain.
These asynchronous continuation transactions will be started after a initiating transaction has been processed and the engine has been released.
If the transaction that initiated a continuation is synchronous, it'll wait for their result; this also extends to non-transactional input like `Set` or `Push`.
If the initiating transaction is asynchronous, it'll pass on its own transaction status to the continuation transactions.

This may sound more complicated than it is. Here's an example:
{% highlight C++ %}
REACTIVE_DOMAIN(D, sequential_concurrent)

VarSignalT<int> Counter = MakeVar<D>(0);

Continuation<D> Cont = MakeContinuation(Counter, [&] (int v) {
    cout << v << endl;
    if (v < 10)
        Counter <<= v + 1;
});
{% endhighlight %}
{% highlight C++ %}
Counter <<= 1;
// output: 1 2 3 4 5 6 7 8 9 10
{% endhighlight %}

Here, the continuation source and target domains are identical and we can add imperative input without deadlocks, while still being able to wait until all recursive continuations complete.

Another example:
{% highlight C++ %}
REACTIVE_DOMAIN(L, sequential_concurrent)
REACTIVE_DOMAIN(R, sequential_concurrent)


VarSignalT<int> CounterL = MakeVar<L>(0);
VarSignalT<int> CounterR = MakeVar<R>(0);

Continuation<L,R> ContL = MakeContinuation<L,R>(
    Monitor(CounterL),
    [&] (int v, int depL1, int depL2) {
        cout << "L->R: " << v << endl;
        if (v < 10)
            srcR <<= v+1;
    });

Continuation<R,L> ContR = MakeContinuation<R,L>(
    Monitor(CounterR),
    [&] (int v) {
        cout << "R->L: " << v << endl;
        if (v < 10)
            srcL <<= v+1;
    });
}
{% endhighlight %}
{% highlight C++ %}
TransactionStatus status;

AsyncTransaction<L>(status, [&] {
	CounterL <<= 1;
});
// output: L->R 1, R->L 2, L->R 3, ... R->L 10

status.Wait();
{% endhighlight %}

Here, two domains bounce messages each off each other repeatedly. Continuations enable inter-domain communication without risk of deadlocks.
It is, however, up to the programmer to ensure that there are no "infinite loops".

For complexity reasons, continuations should not be used to introduce strong coupling between domains, in particular w.r.t. to pre- and post conditions of inputs.