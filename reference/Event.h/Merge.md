---
layout: default
title: Merge
type_tag: function
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Event.h, url: 'reference/Event.h/'}
---
## Syntax
{% highlight C++ %}
template
<
    typename D,
    typename E,
    typename ... TValues
>
TempEvents<D,E,/*unspecified*/>
    Merge(const Events<D,E>& arg1, const Events<D,TValues>& ... args);
{% endhighlight %}

## Semantics
Emit all events in `arg1, ... args`.

## Graph
<img src="{{ site.baseurl }}/media/flow_merge.png" alt="Drawing" width="500px"/>