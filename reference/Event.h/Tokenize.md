---
layout: default
title: Flatten
type_tag: function
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Event.h, url: 'reference/Event.h/'}
---
Utility function to transform any event stream into a token stream.

## Syntax
{% highlight C++ %}
template <typename TEvents>
auto Tokenize(TEvents&& source)
    -> decltype(Transform(source, Tokenizer())));
{% endhighlight %}

## Semantics

Emits a token for any event that passes `source`.

The following helper class is used as a functor for `Transform`:
{% highlight C++ %}
struct Tokenizer
{
    template <typename T>
    Token operator()(const T&) const { return Token::value; }
};
{% endhighlight %}