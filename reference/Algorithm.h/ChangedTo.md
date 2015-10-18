---
layout: default
title: ChangedTo
type_tag: function
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Algorithm.h, url: 'reference/Algorithm.h/'}
---

Emits a token when the target signal was changed to a specific value.

## Syntax
{% highlight C++ %}
template
<
    typename D,
    typename V,
    typename S = decay<V>::type
>
Events<D,Token> ChangedTo(const Signal<D,S>& target, V&& value);
{% endhighlight %}

## Semantics
Creates a token stream that emits when `target` is changed and `target.Value() == value`.

`V` and `S` should be comparable with `==`.