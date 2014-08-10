---
layout: default
title: EventRange
type_tag: class
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Event.h, url: 'reference/Event.h/'}
---
This class represents a range of events. It it serves as an adaptor to the underlying event container of a source node.

An instance of `EventRange` holds a reference to the wrapped container and selectively exposes functionality that allows to iterate over its events without modifying it.

## Synopsis
{% highlight C++ %}
template <typename E>
class EventRange
{
public:
    using const_iterator =  typename /*adaptee<E>*/::const_iterator;
    using size_type =       typename /*adaptee<E>*/::size_type;

    // Constructor
    EventRange(const EventRange&); // Copy

    // Assignment
    EventRange& operator=(const EventRange&); // Copy

    // Returns random access const_iterators to beginning/end of the range
    const_iterator begin() const;
    const_iterator end() const;

    // Returns the number of events in the range
    size_type Size() const;

    // Returns true, if the range is empty
    bool IsEmpty() const;
};
{% endhighlight %}