---
layout: default
title: DoTransaction
type_tag: function
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Domain.h, url: 'reference/Domain.h/'}
---
Executes given function as transaction in the current thread.

## Syntax
{% highlight C++ %}
// (1)
template
<
    typename D,
    typename F
>
void DoTransaction(F&& func);

// (2)
template
<
    typename D,
    typename F
>
void DoTransaction(TransactionFlags flags, F&& func);
{% endhighlight %}

## Semantics
Runs `func` in transaction mode. During transaction mode, all reactive inputs are collected and propagated in a single turn after `func` returns.

The purpose of this is

* to ensure consistency, if there exist certain invariants between reactive inputs;
* to avoid redundant re-calculations;
* to avoid the overhead of additional turns.

(1) uses the default turn flags for the transaction. (2) allows to set flags explicitly.
Valid flag masks result from bitwise disjunction of

Flags are a bitmask of type
{% highlight C++ %}    
using TransactionFlagsT = underlying_type<ETransactionFlags>::type;
{% endhighlight C++ %}

It can be created by Bitwise Or of
{% highlight C++ %}
enum ETransactionFlags
{
    allow_merging
};
{% endhighlight C++ %}

`allow_merging` allows merging of multiple transactions into a single turn to improve performance.
This can happen when N successively mergeable transactions are waiting to gain exclusive access to the graph.
Further, this is only the case for domains that support concurrent input.

The implications are that 

* unrelated inputs could be grouped together, and that
* intermediate signal changes could be ignored, because only the last one is applied.

If these conditions are acceptable, merging can improve concurrent throughput, because

* the overhead associated with each turn is reduced;
* unecessary signal changes are avoided;
* the workload per turn is increased, which generally has a positive effect on parallelization.

Merging happens independently of whether transactions are synchronous or asynchrounous.
Except for the aforementioned implications, it has no further observable effects, in particular w.r.t. to `TransactionStatus` or continuations.