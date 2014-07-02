---
layout: default
title: ScopedObserver
type_tag: class
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Observer.h, url: 'reference/Observer.h/'}
---
Takes ownership of an observer and automatically detaches it on scope exit.

## Synopsis
{% highlight C++ %}
template <typename D>
class ScopedObserver
{
public:
    // Constructor
    ScopedObserver(Observer<D>&& obs);

    // Assignemnt
    Observer& operator=(Observer&& other)
};
{% endhighlight %}