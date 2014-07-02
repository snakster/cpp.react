---
layout: default
title: Domain.h
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
---
Contains the base class for reactive domains and the macro to define them.

## Header
`#include "react/Domain.h"`

## Synopsis

### Constants
{% highlight C++ %}
namespace react
{
    enum EDomainMode
    {
        sequential,
        sequential_concurrent,
        parallel,
        parallel_concurrent
    };

    enum ETurnFlags
    {
        allow_merging
    };

    enum EInputMode;
    enum EPropagationMode;
}
{% endhighlight %}

### Classes
{% highlight C++ %}
namespace react
{
    // DomainBase
    class DomainBase<D>;

    // Continuation
    class Continuation<D,D2>;

    // TransactionStatus
    class TransactionStatus;
}
{% endhighlight %}

### Functions
{% highlight C++ %}
namespace react
{
    using TurnFlagsT = underlying_type<ETurnFlags>::type;

    // DoTransaction
    void DoTransaction<D>(F&& func);
    void DoTransaction<D>(TurnFlagsT flags, F&& func);

    // AsyncTransaction
    void AsyncTransaction<D>(F&& func);
    void AsyncTransaction<D>(TurnFlagsT flags, F&& func);
    void AsyncTransaction<D>(TransactionStatus& status, F&& func);
    void AsyncTransaction<D>(TurnFlagsT flags, TransactionStatus& status, F&& func);

    // Creates a continuation from domain D to D2
    Continuation<D,D2>
        MakeContinuation<D,D2>(const Signal<D,S>& trigger, F&& func);
    Continuation<D,D2>
        MakeContinuation<D,D2>(const Events<D,E>& trigger, F&& func);
    Continuation<D,D2>
        MakeContinuation<D,D2>(const Events<D,E>& trigger,
                               const SignalPack<D,TDepValues...>& depPack, F&& func);
}
{% endhighlight %}

### Macros
{% highlight C++ %}
#define REACTIVE_DOMAIN(name, ...)

#define USING_REACTIVE_DOMAIN(name)
{% endhighlight %}