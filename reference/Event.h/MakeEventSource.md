---
layout: default
title: MakeEventSource
type_tag: function
---
## Syntax
{% highlight C++ %}
// (1)
template <typename D, typename E>
EventSource<D,E> MakeEventSource();

// (2)
template <typename D>
EventSource<D,Token> MakeEventSource();
{% endhighlight %}

## Semantics
Creates a new event source node and links it to the returned `EventSource` instance.

For (1), the event value type `E` has to be specified explicitly.
If it's omitted, (2) will create a token source.

## Graph
<img src="{{ site.baseurl }}/media/flow_makesource.png" alt="Drawing" width="500px"/>