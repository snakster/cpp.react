---
layout: default
---
# Event.h
Contains the event stream template classes and functions.

## Header
`#include "react/Event.h"`

## Synopsis

### Classes
{% highlight C++ %}
namespace react
{
    // Token
    enum class Token;

    // Events
    class Events<D,E=Token>;

    // EventSource
    class EventSource<D,E=Token>;

    // TempEvents
    class TempEvents<D,E,/*unspecified*/>;
}
{% endhighlight %}

### Functions
{% highlight C++ %}
namespace react
{
    // Creates a new event source
    EventSource<D,E> MakeEventSource<D,E=Token>();

    // Creates a new event stream that merges events from given streams
    TempEvents<D,E,/*unspecified*/>
        Merge(const Events<D,E>& arg1, const Events<D,TValues>& ... args);

    // Creates a new event stream that filters events from other stream
    TempEvents<D,E,/*unspecified*/>
        Filter(const Events<D,E>& source, F&& func);
    TempEvents<D,E,/*unspecified*/>
        Filter(TempEvents<D,E,/*unspecified*/>&& source, F&& func);
    Events<D,E>
        Filter(const Events<D,E>& source,
               const SignalPack<D,TDepValues...>& depPack, FIn&& func);

    // Creates a new event stream that transforms events from other stream
    TempEvents<D,T,/*unspecified*/>
        Transform(const Events<D,E>& source, F&& func);
    TempEvents<D,T,/*unspecified*/>
        Transform(TempEvents<D,E,/*unspecified*/>&& source, F&& func);
    Events<D,T>
        Transform(const Events<D,TIn>& source,
                  const SignalPack<D,TDepValues...>& depPack, FIn&& func);

    // Creates a new event stream by flattening a signal of an event stream
    Events<D,T> Flatten(const Signal<D,Events<D,T>>& other);

    // Creates a new event stream by flattening a signal of an event stream
    Events<D,Token> Flatten(const Signal<D,Events<D,T>>& other);

    // Creates a token stream by transforming values from source to tokens
    TempEvents<D,Token,/*unspecified*/>
        Tokenize(TEvents&& source);
}
{% endhighlight %}