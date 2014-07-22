---
layout: default
title: Tokenize
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
TempEvents<D,Token> Tokenize(TEvents&& source)
{% endhighlight %}

## Semantics

Emits a token for any event that passes `source`.

Equivalent to `Transform(source, Tokenizer())` with
{% highlight C++ %}
struct Tokenizer
{
    template <typename T>
    Token operator()(const T&) const { return Token::value; }
};
{% endhighlight %}