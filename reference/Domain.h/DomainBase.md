---
layout: default
title: DomainBase
type_tag: class
---
This class contains the functionality for domains defined with `REACTIVE_DOMAIN`.
A domain cannot be instantiated. It is used to group domain-specific type aliases and static functions.

## Synopsis
{% highlight C++ %}
namespace react
{
    template <typename D>
    class DomainBase
    {
    public:
        DomainBase() = delete;

        // Type aliases for this domain
        template <typename S>
        using SignalT = Signal<D,S>;

        template <typename S>
        using VarSignalT = VarSignal<D,S>;

        template <typename E = Token>
        using EventsT = Events<D,E>;

        template <typename E = Token>
        using EventSourceT = EventSource<D,E>;

        using ObserverT = Observer<D>;

        using ReactorT = Reactor<D>;
    };
}
{% endhighlight %}

## Member functions

### (Constructor)
#### Syntax
``` C++
DomainBase() = delete;
```

#### Semantics
Deleted default constructor to prevent instantiation.