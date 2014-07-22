---
layout: default
title: DomainBase
type_tag: class
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Domain.h, url: 'reference/Domain.h/'}
---
This class is used as a base class of all domains defined with `REACTIVE_DOMAIN`.
It defines alias types for reactives of the derived domain.
A domain cannot be instantiated.
It's sole purpose is to serve as a type tag for reactive values and domain-specific internal data.

## Synopsis
{% highlight C++ %}
template <typename D>
class DomainBase
{
public:
    // Deleted default constructor to prevent instantiation
    DomainBase() = delete;

    // Type aliases for derived domain D
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
{% endhighlight %}