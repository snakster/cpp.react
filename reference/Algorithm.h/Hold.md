---
layout: default
title: Hold
type_tag: function
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Algorithm.h, url: 'reference/Algorithm.h/'}
---
## Syntax
{% highlight C++ %}
template
<
    typename D,
    typename V,
    typename T = decay<V>::type
>
Signal<D,T> Hold(const Events<D,T>& events, V&& init);
{% endhighlight %}

## Semantics
Creates a signal with an initial value `v = init`.
For received event values `e1, e2, ... eN` in `events`, it is updated to `v = eN`.