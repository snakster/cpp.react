---
layout: default
title: TempSignal
type_tag: class
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Signal.h, url: 'reference/Signal.h/'}
---
This class exposes additional type information of the linked node, which enables r-value based node merging at construction time.
The primary use case for this is to avoid unnecessary nodes when creating signal expression from overloaded arithmetic operators.

`TempSignal` is usually not used as an l-value type, but instead implicitly converted to `Signal`.

##### Synopsis
{% highlight C++ %}
template
<
    typename D,
    typename S,
    typename TOp
>
class TempSignal : public Signal<D,S>
{
public:
    // Constructor
    TempSignal();
    TempSignal(const TempSignal&);
    TempSignal(TempSignal&&);

    // Assignment
    TempSignal& operator=(const TempSignal&);
    TempSignal& operator=(TempSignal&& other);
};
{% endhighlight %}

-----

<h1>Constructor, operator= <span class="type_tag">member function</span></h1>

Analogously defined to [Signal](Signal.html).