### Header
`#include "react/Signal.h"`

### Summary
---
layout: default
---

Contains the signal template classes and functions.

* [Classes](#classes)
  - [Signal](#signal)
  - [VarSignal](#varsignal)
  - [TempSignal](#tempsignal)
* [Functions](#functions)
  - [MakeVar](#makevar)
  - [MakeSignal](#makesignal)
  - [Flatten](#flatten)
* [Operators](#operators)

# Classes

## Signal

A signal is a reactive variable that can propagate its changes to dependents and react to changes of its dependencies.
Dataflow between signals is modeled as a directed acyclic graph, where each signal is a node and edges denote a dependency relation.
If an edge exists from `v1` to `v2` that means `v1` will propagate its changes to `v2`.
In other words, after a node changed, all its sucessors will be updated.

The following example shows the dataflow graph for a given code snippet:
{% highlight C++ %}
SignalT<int> a = ...;
SignalT<int> b = ...;
SignalT<int> c = ...;
SignalT<int> x = (a + b) * c;
{% endhighlight %}
<br />
<img src="images/signals1.png" alt="Drawing" width="500px" />

The graph data structures are not directly exposed by the API; instead, instances of class `Signal` (and its subtypes) act as lightweight proxies to nodes.
Such a proxy is essentially a shared pointer to a heap-allocated node, with additional interface methods depending on the concrete type.
Copy, move and assignment semantics are similar to `std::shared_ptr`.
One observation made from the previous example is that not all nodes in the graph are named signals; the temporary sub-expression `a + b` results in a node as well.
If a new node is created, it takes shared ownership of its dependencies, because it needs them to calculate its own value. This prevents the `a + b` node from disappearing.

The resulting reference graph is similar to the dataflow graph, but with reverse edges (and as such, a DAG as well): <br />
<img src="images/signals2.png" alt="Drawing" width="500px" />

The number inside each node denotes its reference count. On the left are the proxy instances exposed by the API.
Assuming the handles for `a`, `b` and `c` would go out of scope but `x` remains, the reference count of all nodes is still 1, until `x` disappears as well.
Once that happens, the graph is deconstructed from the bottom up.

`Signal` erases the exact type of signal node it points to. It's only parametrized with the domain and the type of stored values.
It provides read-only access to the current signal value, which is part of the minimal interface all signal nodes have in common.

In general, the update procedure of a signal node is implemented like this:
{% highlight C++ %}
void Update()
{
    S newValue = /* calc new value */

    if (currentValue != newValue)
    {
        currentValue = std::move(newValue);
        D::PropagationEngine::OnChange(*this);
    }
}
{% endhighlight %}
The propagation engine is responsible for calling `Update` on successors.

Constructing a `Signal` instance and linking it to a a node is done with `MakeSignal` or other operations that returns a signal.

##### Synopsis
{% highlight C++ %}
namespace react
{
    template
    <
        typename D,
        typename S
    >
    class Signal
    {
    public:
        using ValueT = S;

        // Constructor
        Signal();
        Signal(const Signal&);  // Copy
        Signal(Signal&&);       // Move

        // Assignment
        Signal& operator=(const Signal&);   // Copy
        Signal& operator=(Signal&& other);  // Move

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
}
{% endhighlight %}

### Template parameters
<table>
<tr>
<td>D</td>
<td>The domain this signal belongs to.</td>
</tr>
<tr>
<td>S</td>
<td>Signal value type. Aliased as member type <code>ValueT</code>.</td>
</tr>
</table>

### Member functions

#### (Constructor)
###### Syntax
{% highlight C++ %}
Signal();                    // (1)
Signal(const Signal& other); // (2)
Signal(Signal&& other);      // (3)
{% endhighlight %}
###### Semantics
(1) Creates an invalid signal that is not linked to a signal node.

(2) Creates a signal that links to the same signal node as `other`.

(3) Creates a signal that moves shared ownership of the signal node from `other` to `this`.
As a result, `other` becomes invalid.

**Note:** The default constructor creates an invalid signal, which is equivalent to `std::shared_ptr(nullptr)`.
This is potentially dangerous and considering the declarative nature of signals, it should be avoided if possible.

#### Equals
###### Syntax
{% highlight C++ %}
bool Equals(const Signal& other) const;
{% endhighlight %}

###### Semantics
Returns true, if both `this` and `other` link to the same signal node.
This function is used to compare two signals, because `==` is used as a combination operator instead.

#### IsValid
###### Syntax
{% highlight C++ %}
bool IsValid() const;
{% endhighlight %}

###### Semantics
Returns true, if `this` is linked to a signal node.

#### Value, operator `()`
###### Syntax
{% highlight C++ %}
const S& Value() const;
const S& operator()() const;
{% endhighlight %}

###### Semantics
Returns a const reference to the current signal value.

#### Flatten
###### Syntax
{% highlight C++ %}
S Flatten() const;
{% endhighlight %}

###### Semantics
Semantically equivalent to the respective free function in namespace `react`.

## VarSignal
`VarSignal` extends the immutable `Signal` interface with functions that support imperative value input.
In the dataflow graph, input signals are sources. As such, they don't have any predecessors.

##### Synopsis
{% highlight C++ %}
namespace react
{
    template
    <
        typename D,
        typename S
    >
    class VarSignal : public Signal<D,S>
    {
    public:
        // Constructor
        VarSignal();
        VarSignal(const VarSignal&);
        VarSignal(VarSignal&&);

        // Assignment
        VarSignal& operator=(const VarSignal&);
        VarSignal& operator=(VarSignal&& other);

        // Set new signal value
        void Set(const S& newValue);
        void Set(S&& newValue);

        // Operator version of Set
        const VarSignal& operator<<=(const S& newValue);
        const VarSignal& operator<<=(S&& newValue);

        // Modify current signal value in-place
        void Modify(const F& func);
    };
}
{% endhighlight %}

### Member functions

#### (Constructor), operator `=`
Analogously defined to [Signal](#signal).

#### Set
###### Syntax
{% highlight C++ %}
void Set(const S& newValue) const;
void Set(S&& newValue) const;
{% endhighlight %}

###### Semantics
Set the the signal value of the linked variable signal node to `newValue`. If the old value equals the new value, the call has no effect.

Furthermore, if `Set` was called inside of a transaction function, it will return after the changed value has been set and change propagation is delayed until the transaction function returns.
Otherwise, propagation starts immediately and `Set` blocks until it's done.

#### Operator `<<=`
###### Syntax
{% highlight C++ %}
const VarSignal& operator<<=(const S& newValue);
const VarSignal& operator<<=(S&& newValue);
{% endhighlight %}

###### Semantics
Semantically equivalent to `Set`.

#### Modify
###### Syntax
{% highlight C++ %}
template <typename F>
void Modify(const F& func);
{% endhighlight %}

###### Semantics
TODO

## TempSignal
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
namespace
{
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
}
{% endhighlight %}

### Member functions

#### (Constructor), operator `=`
Analogously defined to [Signal](#signal).

# Functions

##### Synopsis
{% highlight C++ %}
namespace react
{
    //Creates a new variable signal
    VarSignal<D,S> MakeVar(V&& init);

    // Creates a signal as a function of other signals
    TempSignal<D,S,/*unspecified*/>
        MakeSignal(F&& func, const Signal<D,TValues>& ... args);

    // Creates a new signal by flattening a signal of a signal
    Signal<D,T> Flatten(const Signal<D,Signal<D,T>>& other);
} 
{% endhighlight %}

#### MakeVar
###### Syntax
{% highlight C++ %}
template
<
    typename D,
    typename V,
    typename S = decay<V>::type,
>
VarSignal<D,S> MakeVar(V&& init);
{% endhighlight %}

###### Semantics
Creates a new input signal node and links it to the returned `VarSignal` instance.

###### Graph
<img src="images/flow_makevar.png" alt="Drawing" width="500px"/>

#### MakeSignal
###### Syntax
{% highlight C++ %}
template
<
    typename D,
    typename F,
    typename ... TValues,
    typename S = result_of<F(TValues...)>::type
>
TempSignal<D,S,/*unspecified*/>
    MakeSignal(F&& func, const Signal<D,TValues>& ... args);
{% endhighlight %}

###### Semantics
Creates a new signal node with value `v = func(args.Value(), ...)`.
This value is set on construction and updated when any `args` have changed.

The signature of `func` should be equivalent to:

* `S func(const TValues& ...)`

###### Graph
<img src="images/flow_makesignal.png" alt="Drawing" width="500px"/>

#### Flatten
###### Syntax
{% highlight C++ %}
template
<
    typename D,
    typename T
>
Signal<D,T> Flatten(const Signal<D,Signal<D,T>>& other);
{% endhighlight %}

###### Semantics
TODO

###### Graph
<img src="images/flow_flatten.png" alt="Drawing" width="500px"/>

# Operators

##### Synopsis
{% highlight C++ %}
namespace react
{
    //
    // Overloaded unary operators
    //      Arithmetic:     +   -   ++  --
    //      Bitwise:        ~
    //      Logical:        !
    //

    // OP <Signal>
    TempSignal<D,S,/*unspecified*/>
        OP(const TSignal& arg)

    // OP <TempSignal>
    TempSignal<D,S,/*unspecified*/>
        OP(TempSignal<D,TVal,/*unspecified*/>&& arg);

    //
    // Overloaded binary operators
    //      Arithmetic:     +   -   *   /   %
    //      Bitwise:        &   |   ^
    //      Comparison:     ==  !=  <   <=  >   >=
    //      Logical:        &&  ||
    //
    // NOT overloaded:
    //      Bitwise shift:  <<  >>
    //


    // <Signal> BIN_OP <Signal>
    TempSignal<D,S,/*unspecified*/>
        BIN_OP(const TLeftSignal& lhs, const TRightSignal& rhs)

    // <Signal> BIN_OP <NonSignal>
    TempSignal<D,S,/*unspecified*/>
        BIN_OP(const TLeftSignal& lhs, TRightVal&& rhs);

    // <NonSignal> BIN_OP <Signal>
    TempSignal<D,S,/*unspecified*/>
        BIN_OP(TLeftVal&& lhs, const TRightSignal& rhs);

    // <TempSignal> BIN_OP <TempSignal>
    TempSignal<D,S,/*unspecified*/>
        BIN_OP(TempSignal<D,TLeftVal,/*unspecified*/>&& lhs,
               TempSignal<D,TRightVal,/*unspecified*/>&& rhs);

    // <TempSignal> BIN_OP <Signal>
    TempSignal<D,S,/*unspecified*/>
        BIN_OP(TempSignal<D,TLeftVal,/*unspecified*/>&& lhs,
               const TRightSignal& rhs);

    // <Signal> BIN_OP <TempSignal>
    TempSignal<D,S,/*unspecified*/>
        BIN_OP(const TLeftSignal& lhs,
               TempSignal<D,TRightVal,/*unspecified*/>&& rhs)

    // <TempSignal> BIN_OP <NonSignal>
    TempSignal<D,S,/*unspecified*/>
        BIN_OP(TempSignal<D,TLeftVal,/*unspecified*/>&& lhs,
               TRightVal&& rhs);

    // <NonSignal> BIN_OP <TempSignal>
    TempSignal<D,S,/*unspecified*/>
        BIN_OP(TLeftVal&& lhs,
               TempSignal<D,TRightVal,/*unspecified*/>&& rhs);
}
{% endhighlight %}