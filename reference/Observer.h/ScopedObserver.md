---
layout: default
title: ScopedObserver
type_tag: class
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Observer.h, url: 'reference/Observer.h/'}
---
Takes ownership of an observer and automatically detaches it on scope exit.

## Synopsis
{% highlight C++ %}
template <typename D>
class ScopedObserver
{
public:
    // Constructs instance from observer
    ScopedObserver(Observer<D>&& obs);

    // Move constructor
    ScopedObserver(ScopedObserver&& other);

    // Destructor
    ~ScopedObserver();

    // Move assignment
    ScopedObserver& operator=(ScopedObserver&& other);

    // Tests if this instance is linked to a node
    bool IsValid() const;

    // Sets weight override for linked node
    void SetWeightHint(WeightHint hint);
};
{% endhighlight %}