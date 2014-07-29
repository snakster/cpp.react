---
layout: default
title: SignalPack
type_tag: class
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Signal.h, url: 'reference/Signal.h/'}
---
A wrapper type for a tuple of signal references.

`SignalPacks` are created by [With](With.html).

## Synopsis
{% highlight C++ %}
template
<
    typename D,
    typename ... TValues
>
class SignalPack
{
public:
    // Construct from signals
    SignalPack(const Signal<D,TValues>&  ... deps);

    // Construct by appending signal to other SignalPack
    template
    <
        typename ... TCurValues,
        typename TAppendValue
    >
    SignalPack(const SignalPack<D, TCurValues ...>& curArgs, const Signal<D,TAppendValue>& newArg);

    // The wrapped tuple
    std::tuple<const Signal<D,TValues>& ...> Data;
};
{% endhighlight %}