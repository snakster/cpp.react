---
layout: default
title: Observers
groups: 
 - {name: Home, url: ''}
 - {name: Tutorials , url: 'tutorials/'}
---

- [Creating subject-bound observers](#creating-subject-bound-observers)
- [Detaching observers manually](#detach-observers-manually)
- [Using scoped observers](#using-scoped-observers)
- [Observing temporary reactives](#observing-temporary-reactives)

## Creating subject-bound observers

As mentioned in the previous examples, by default observers are bound to the lifetime of the observed subject.
Here's another demonstration of this behaviour:
{% highlight C++ %}
#include "react/Domain.h"
#include "react/Event.h"
#include "react/Observer.h"

REACTIVE_DOMAIN(D, sequential)
USING_REACTIVE_DOMAIN(D)

void testFunc()
{
    auto trigger = MakeEventSource<D>();

    Observe(trigger, [] {
        // ...
    });
}
{% endhighlight %}
After leaving `testFunc`, `trigger` is destroyed.
This automatically detaches and destroys the observer as well.
We don't have to worry about resource leaks or that the existence of an observer might prevent the subject from being destroyed.


## Detaching observers manually

Alternatively, observers can be detached explicitly before the lifetime of their subject ends.
We can either do this externally, or from inside the observer function.

The former is demonstrated here:
{% highlight C++ %}
void testFunc()
{
    auto trigger = MakeEventSource<D>();

    ObserverT obs = Observe(trigger, [] (Token) {
        cout << "Triggered" << endl;
    });

    trigger.Emit(); // output: Triggered

    obs.Detach();   // Remove the observer

    trigger.Emit(); // no output
}
{% endhighlight %}

`ObserverT` is an alias for `Observer<D>` defined by `USING_REACTIVE_DOMAIN(D)`.
The observer handle returned by `Observe` is stored and by calling its `Detach` member function, the underlying observer node is destroyed and the handle becomes invalid.

While it exists and has not been invalidated, an observer handle also takes shared ownership of the observed subject, i.e. by saving it, we state our continued interest in subject.
This behaviour comes in handy as shown later.

To create self-detaching observers, the return value of the observer function is changed from  `void` to `ObserverAction`:
{% highlight C++ %}
void testFunc()
{
    auto source = MakeEventSource<D,int>();

    Observe(trigger, [] (int v) -> ObserverAction {
        if (v != 0)
        {
            cout << v << endl;
            return ObserverAction::next;
        }
        else
        {
            cout << "Detached" << endl;
            return ObserverAction::stop_and_detach;
        }
    });

    source << 3 << 2 << 1 << 0;
    // output: 3 2 1 Detached

    source << 4;
    // no output
}
{% endhighlight %}


## Using scoped observers

Instead of calling `Detach` manually, we can utilize the common RAII idiom and transfer ownership of the observer handle to a scope guard:

{% highlight C++ %}
void testFunc()
{
    auto trigger = MakeEventSource<D>();

    // Start inner scope
    {
        ScopedObserverT scopedObs
        (
            Observe(trigger, [] (Token) {
                cout << "Triggered" << endl;
            })
        );

        trigger.Emit();
        // output: Triggered
    }
    // End inner scope

    trigger.Emit();
    // no output
}
{% endhighlight %}

The observer is detached in the destructor of `ScopedObserverT`.

## Observing temporary reactives

One problem of tying the lifetime of the observer to its subject manifests when attempting to do the following:
{% highlight C++ %}
void testFunc()
{
    auto e1 = MakeEventSource<D>();
    auto e2 = MakeEventSource<D>();

    Observe(Merge(e1, e2), [] (Token) {
        cout << "Triggered!" << endl;
    });

    e1.Emit(); // no output
    e2.Emit(); // no output
}
{% endhighlight %}
The reason this produces no output is that the result of `Merge(e1, e2)` is destroyed after the call, as nobody takes ownership of it.
Consequently, the observer is destroyed as well.

To prevent this, the merged event stream could be kept in variable.
Alternatively, we can make use of the fact that the observer handle takes ownership of its subject:
{% highlight C++ %}
auto obs = Observe(Merge(e1, e2), [] (Token) {
    cout << "Triggered" << endl;
});

e1.Emit(); // output: Triggered
e2.Emit(); // output: Triggered
{% endhighlight %}