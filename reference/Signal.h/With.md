---
layout: default
title: With
type_tag: function
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Signal.h, url: 'reference/Signal.h/'}
---
Utility function to create a signal pack.

## Syntax
{% highlight C++ %}
template
<
    typename D,
    typename ... TValues
>
SignalPack<D,TValues ...>
	With(const Signal<D,TValues>&  ... deps);
{% endhighlight %}

## Semantics
Creates a `SignalPack` from the signals passed as `deps`.

Semantically, this is equivalent to `std::tie`.