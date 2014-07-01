---
layout: default
---
# Signal

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