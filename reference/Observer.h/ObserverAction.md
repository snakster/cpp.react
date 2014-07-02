---
layout: default
title: ObserverAction
type_tag: enum class
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Observer.h, url: 'reference/Observer.h/'}
---
Observer functions can return values of this type to control further processing.

## Definition
{% highlight C++ %}
enum class ObserverAction
{
    next,
    stop_and_detach
};
{% endhighlight %}