---
layout: default
title: TypeTraits.h
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
---
Contains traits for compile-time type inspection and transformation.

## Header
{% highlight C++ %}
#include "react/Signal.h"
{% endhighlight %}

## Synopsis

### Classes

{% highlight C++ %}
// True, if T is any signal type
bool IsSignal<T>::value

// True, if T is any event stream type
bool IsEvent<T>::value

// True, if T is any observer type
bool IsObserver<T>::value

// True, if T is any continuation type
bool IsContinuation<T>::value

// True, if T supports Observe()
bool IsObservable<T>::value

// True, if T is any reactive type,
// including signals, event streams, observers and continutations
bool IsReactive<T>::value

// Decays VarSignal/EventSource to Signal/Events, respectively
struct DecayInput<T>
{ 
    using Type = /* ... */
}
{% endhighlight %}