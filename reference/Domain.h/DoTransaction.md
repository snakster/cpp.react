---
layout: default
title: DoTransaction
type_tag: function
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Domain.h, url: 'reference/Domain.h/'}
---
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
void DoTransaction(TurnFlagsT flags, F&& func);
{% endhighlight %}

## Semantics
Runs `func` in transaction mode. During transaction mode, all reactive inputs are collected and propagated in a single turn after `func` returns.

The purpose of this is

* to ensure consistency, if there exist certain invariants between reactive inputs;
* to avoid redundant re-calculations;
* to avoid the overhead of additional turns.

(1) uses the default turn flags for the transaction. (2) allows to set flags explicitly.