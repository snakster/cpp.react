---
layout: default
---
# OnChangedTo

## Syntax
{% highlight C++ %}
template
<
    typename D,
    typename V,
    typename S = decay<V>::type
>
Events<D,Token> OnChangedTo(const Signal<D,S>& target, V&& value);
{% endhighlight %}

## Semantics
Creates a token stream that emits when `target` is changed and `target.Value() == value`.

`V` and `S` should be comparable with `==`.