---
layout: default
title: Changed
type_tag: function
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Algorithm.h, url: 'reference/Algorithm.h/'}
---

Emits a token when the target signal was changed.

## Syntax
{% highlight C++ %}
template
<
    typename D,
    typename S
>
Events<D,Token> Changed(const Signal<D,S>& target);
{% endhighlight %}

## Semantics
Creates a token stream that emits when `target` is changed.
