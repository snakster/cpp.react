---
layout: default
---
# Algorithm.h

Contains various reactive operations to combine and convert signals and events.

## Header
`#include "react/Algorithm.h"`

## Synopsis

### Functions
{% highlight C++ %}
namespace react
{
    // Iteratively combines signal value with values from event stream
    Signal<D,S> Iterate(const Events<D,E>& events, V&& init, F&& func);
    Signal<D,S> Iterate(const Events<D,E>& events, V&& init,
                        const SignalPack<D,TDepValues...>& depPack, FIn&& func);

    // Hold the most recent event in a signal
    Signal<D,T> Hold(const Events<D,T>& events, V&& init);

    // Sets signal value to value of target when event is received
    Signal<D,S> Snapshot(const Events<D,E>& trigger, const Signal<D,S>& target);

    // Emits value changes of target signal
    Events<D,S> Monitor(const Signal<D,S>& target);

    // Emits value of target signal when event is received
    Events<D,S> Pulse(const Events<D,E>& trigger, const Signal<D,S>& target);

    // Emits token when target signal was changed
    Events<D,Token> OnChanged(const Signal<D,S>& target);

    // Emits token when target signal was changed to value
    Events<D,Token> OnChangedTo(const Signal<D,S>& target, V&& value);
}
{% endhighlight %}