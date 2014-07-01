---
layout: default
---
# Observer.h
Contains the observer template classes and functions.

## Header
`#include "react/Observer.h"`

## Synopsis

### Constants
{% highlight C++ %}
namespace react
{
    enum class ObserverAction;
}
{% endhighlight %}

### Classes
{% highlight C++ %}
namespace react
{
    // Observer
    class Observer<D>;

    // ScopedObserver
    class ScopedObserver<D>;
}
{% endhighlight %}

### Functions
{% highlight C++ %}
namespace react
{
    // Creates an observer of the given subject
    Observer<D> Observe(const Signal<D,S>& subject, F&& func);
    Observer<D> Observe(const Events<D,E>& subject, F&& func);
    Observer<D> Observe(const Events<D,E>& subject,
                        const SignalPack<D,TDepValues...>& depPack, F&& func);
}
{% endhighlight %}