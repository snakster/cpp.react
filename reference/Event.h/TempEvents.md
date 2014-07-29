---
layout: default
title: TempEvents
type_tag: class
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Event.h, url: 'reference/Event.h/'}
---
This class exposes additional type information of the linked node, which enables r-value based node merging at compile time.
`TempEvents` is usually not used as an l-value type, but instead implicitly converted to `Events`.

## Synopsis
{% highlight C++ %}
template
<
    typename D,
    typename E,
    typename TOp
>
class TempEvents : public Events<D,E>
{
public:
    // Constructor
    TempEvents();
    TempEvents(const TempEvents&);  // Copy
    TempEvents(TempEvents&&);       // Move

    // Assignemnt
    TempEvents& operator=(const TempEvents&);   // Copy
    TempEvents& operator=(TempEvents&& other);  // Move
};
{% endhighlight %}


-----

<h1>Constructor, operator= <span class="type_tag">member function</span></h1>

Analogously defined to [Events](Events.html).