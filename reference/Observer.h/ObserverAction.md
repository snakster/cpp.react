---
layout: default
title: ObserverAction
type_tag: enum class
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