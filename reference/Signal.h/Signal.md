---
layout: default
title: Signal
type_tag: class
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Signal.h, url: 'reference/Signal.h/'}
---
A signal is a reactive variable that can propagate its changes to dependents and react to changes of its dependencies.

Instances of this class act as a proxies to  signal nodes.
It takes shared ownership of the node, so while it exists, the node will not be destroyed.
Copy, move and assignment semantics are similar to `std::shared_ptr`.

## Synopsis
{% highlight C++ %}
template
<
    typename D,
    typename S
>
class Signal
{
public:
    using DomainT = D;
    using ValueT = S;

    // Constructor
    Signal();
    Signal(const Signal&);
    Signal(Signal&&);

    // Assignment
    Signal& operator=(const Signal&);
    Signal& operator=(Signal&& other);

    // Tests if two Signal instances are equal
    bool Equals(const Signal& other) const;

    // Tests if this instance is linked to a node
    bool IsValid() const;

    // Returns the current signal value
    const S& Value() const;
    const S& operator()() const;

    // Equivalent to react::Flatten(*this)
    S Flatten() const;
};
{% endhighlight %}

## Template parameters
<table class="wide_table">
    <tr>
        <td class="descriptor_cell">D</td>
        <td>The domain this signal belongs to. Aliases as member type <code>DomainT</code></td>
    </tr>
    <tr>
        <td class="descriptor_cell">E</td>
        <td>Signal value type. Aliased as member type <code>ValueT</code>.</td>
    </tr>
</table>

-----

<h1>Constructor <span class="type_tag">member function</span></h1>

## Syntax
{% highlight C++ %}
Signal();                    // (1)
Signal(const Signal& other); // (2)
Signal(Signal&& other);      // (3)
{% endhighlight %}

## Semantics
(1) Creates an invalid signal that is not linked to a signal node.

(2) Creates a signal that links to the same signal node as `other`.

(3) Creates a signal that moves shared ownership of the signal node from `other` to `this`.
As a result, `other` becomes invalid.

**Note:** The default constructor creates an invalid signal, which is equivalent to `std::shared_ptr(nullptr)`.
This is potentially dangerous and considering the declarative nature of signals, it should be avoided if possible.

-----

<h1>Equals <span class="type_tag">member function</span></h1>

## Syntax
{% highlight C++ %}
bool Equals(const Signal& other) const;
{% endhighlight %}

## Semantics
Returns true, if both `this` and `other` link to the same signal node.
This function is used to compare two signals, because `==` is used as a combination operator instead.

-----

<h1>IsValid <span class="type_tag">member function</span></h1>

## Syntax
{% highlight C++ %}
bool IsValid() const;
{% endhighlight %}

## Semantics
Returns true, if `this` is linked to a signal node.

-----

<h1>Value, operator() <span class="type_tag">member function</span></h1>

## Syntax
{% highlight C++ %}
const S& Value() const;
const S& operator()() const;
{% endhighlight %}

## Semantics
Returns a const reference to the current signal value.

-----

<h1>Flatten <span class="type_tag">member function</span></h1>

## Syntax
{% highlight C++ %}
S Flatten() const;
{% endhighlight %}

## Semantics
Semantically equivalent to the respective free function in namespace `react`.