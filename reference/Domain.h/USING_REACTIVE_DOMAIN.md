---
layout: default
title: USING_REACTIVE_DOMAIN
type_tag: macro
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Domain.h, url: 'reference/Domain.h/'}
---
## Syntax
{% highlight C++ %}
#define USING_REACTIVE_DOMAIN(name)
{% endhighlight %}

## Semantics
Defines the following type aliases for the given domain name in the current scope:
{% highlight C++ %}
template <typename S>
using SignalT = Signal<name,S>;

template <typename S>
using VarSignalT = VarSignal<name,S>;

template <typename E = Token>
using EventsT = Events<name,E>;

template <typename E = Token>
using EventSourceT = EventSource<name,E>;

using ObserverT = Observer<name>;

using ReactorT = Reactor<name>;
{% endhighlight %}