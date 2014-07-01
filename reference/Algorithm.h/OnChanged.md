---
layout: default
title: OnChanged
type_tag: function
---
## Syntax
{% highlight C++ %}
template
<
    typename D,
    typename S
>
Events<D,Token> OnChangedTo(const Signal<D,S>& target);
{% endhighlight %}

## Semantics
Creates a token stream that emits when `target` is changed.