---
layout: default
---
# ObserverAction

Observer functions can return values of this type to control further processing.

## Definition
{% highlight C++ %}
namespace react
{
    enum class ObserverAction
    {
        next,
        stop_and_detach
    };
}
{% endhighlight %}