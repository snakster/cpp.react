---
layout: default
title: TempSignal
type_tag: class
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Signal.h, url: 'reference/Signal.h/'}
---
This class exposes additional type information of the linked node, which enables r-value based node merging at compile time.

This is motivated by the difference between using `MakeSignal` and implicit lifting with operators:
{% highlight C++ %}
// Implicit lifting
SignalT<int> xLift   = (a + b) * c;

// Explicit MakeSignal
SignalT<int> xFunc = MakeSignal(
    Width(a,b,c),
    [] (int a, int b, int c) {
        (a + b) * c;
    });
{% endhighlight %}
Both versions calculate the same result, but their graph representations not equivalent.
The sub-expression `a + b` results in an extra node with lifting, whereas `xFunc` only creates a single node for the whole expression.
Consequently, lifting allows for more concise code, but comes at the cost of increased graph complexity.
To prevent this, r-value signals are merged to a single node. The type information in `TempSignal` enables this, as illustrated by the following pseudo-code:
{% highlight C++ %}
TempSignalT<int,AddOp<int,int>>
    t = a + b;
TempSignalT<int,MultOp<AddOp<int,int>,int>>
    x = t * c;
{% endhighlight %}
When an r-value `TempSignal` is passed to an operator, it'll move all the data out of the previous node and merge it into the newly created one.
Since the operation is statically encoded in the type, the merged function can be optimized by the compiler.
This results in a small overhead during construction, but mitigates any further cost during node updating.

`TempSignal` is usually not used as an l-value type, but instead implicitly converted to `Signal`.
The following example demonstrates this:
{% highlight C++ %}
SignalT<int> x = (a + b) * c;
{% endhighlight %}
1.  `a + b` returns a `TempSignal` `t`
2. `t` is an r-value, so it gets merged by `t * c`, which returns a `TempSignal` as well.
3. By assigning the `TempSignal` to a `Signal`, the extra type information is erased.

There is one situation, where temporary signals stay around longer; that is, when using `auto`.
This allows to request node merging manually, for example when creating a complex signal with several intermediate signals:
{% highlight C++ %}
// t still has its TempSignal type
auto t = a + b; 

// explicitly merged t into x
auto x = std::move(t) * c;
{% endhighlight %}
Without the `std::move`, there would be no merging, as `t` may be a `TempSignal` but can't be bound to an r-value reference.

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

Analogously defined to [Signal](#signal).