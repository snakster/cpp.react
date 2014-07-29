---
layout: default
title: Observer
type_tag: class
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Observer.h, url: 'reference/Observer.h/'}
---
An instance of this class provides a unique handle to an observer which can be used to detach it explicitly.
It also holds a strong reference to the observed subject, so while it exists the subject and therefore the observer will not be destroyed.

If the handle is destroyed without calling `Detach()`, the lifetime of the observer is tied to the subject.

## Synopsis
{% highlight C++ %}
template <typename D>
class Observer
{
public:
    // Default constructor
    Observer();

    // Move constructor
    Observer(Observer&& other);

    // Move assignemnt
    Observer& operator=(Observer&& other)

    // Tests if this instance is linked to a node
    bool IsValid() const;

    // Sets weight override for linked node
    void SetWeightHint(WeightHint hint);

    // Manually detaches the linked observer node from its subject
    void Detach();
};
{% endhighlight %}