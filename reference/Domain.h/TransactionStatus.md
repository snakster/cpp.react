---
layout: default
title: TransactionStatus
type_tag: class
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Domain.h, url: 'reference/Domain.h/'}
---
This class allow to wait for a asynchrounous transactions started with [AsyncTransaction](AsyncTransaction.html).

An instance of `TransactionStatus` shares ownership of its internal state with any transactions it is monitoring.
The instance itself is a move-only type.
A single `TransactionState` can monitor multiple transactions and it can be re-used to avoid repeated state allocations.

## Synopsis
{% highlight C++ %}
class TransactionStatus
{
    // Constructor
    TransactionStatus();
    TransactionStatus(TransactionStatus&& other);

    // Assignemnt
    TransactionStatus& operator=(TransactionStatus&& other);

    // Waits until any transactions associated with this instance are done
    void Wait();
};
{% endhighlight %}

-----

<h1>Constructor <span class="type_tag">member function</span></h1>

## Syntax
{% highlight C++ %}
TransactionStatus();                            // (1)
TransactionStatus(TransactionStatus&& other);   // (2)
{% endhighlight %}

## Semantics
(1) Creates a status instance that does not monitor any transactions.

(2) Creates a status instance by moving internal state from `other` to this. A new state is allocated for `other`.

-----

<h1>operator= <span class="type_tag">member function</span></h1>

## Syntax
{% highlight C++ %}
TransactionStatus& operator=(TransactionStatus&& other);
{% endhighlight %}

## Semantics
Moves the internal state from `other` to this. Ownership of any previous state is released and a new state is allocated for `other`.

-----

<h1>Wait <span class="type_tag">member function</span></h1>

## Syntax
{% highlight C++ %}
void Wait();
{% endhighlight %}

## Semantics
Blocks the current thread until all monitored transactions are done.
After `Wait` returns, the status instance can be reused.