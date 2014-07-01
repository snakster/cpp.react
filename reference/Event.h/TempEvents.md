---
layout: default
---
# TempEvents

This class exposes additional type information of the linked node, which enables r-value based node merging at compile time.
`TempEvents` is usually not used as an l-value type, but instead implicitly converted to `Events`.

## Synopsis
{% highlight C++ %}
namespace react
{
    template
    <
        typename D,
        typename E,
        typename TOp
    >
    class TempEvents : public Events<D,E>
    {
    public:
        // Constructor
        TempEvents();
        TempEvents(const TempEvents&);
        TempEvents(TempEvents&&);

        // Assignemnt
        TempEvents& operator=(const TempEvents&);
        TempEvents& operator=(TempEvents&& other);
    };
}
{% endhighlight %}

## Member functions

### (Constructor)
Analogously defined to constructor of [Events](#events).