---
layout: default
title: AsyncTransaction
type_tag: function
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Domain.h, url: 'reference/Domain.h/'}
---

Enqueues given function as as asynchronous transaction and returns.

## Syntax
{% highlight C++ %}
// (1)
template
<
    typename D,
    typename F
>
void AsyncTransaction(F&& func);

// (2)
template
<
    typename D,
    typename F
>
void AsyncTransaction(TransactionFlagsT flags, F&& func);

// (3)
template
<
    typename D,
    typename F
>
void AsyncTransaction(TransactionStatus& status, F&& func);

// (4)
template
<
    typename D,
    typename F
>
void AsyncTransaction(TransactionFlagsT flags, TransactionStatus& status, F&& func);
{% endhighlight %}

## Semantics
Similar to `DoTransaction`, but the transaction function is not executed immediately but pushed to the asychronous transaction queue.
Following a producer/consumer scheme, transactions in this queue are processed by a dedicated worker thread.
The calling thread immediately returns.

(3,4) allow to pass an optional `TransactionStatus` instance, which can be used to wait for completetion of the transaction.