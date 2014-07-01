---
layout: default
---
# VarSignal
`VarSignal` extends the immutable `Signal` interface with functions that support imperative value input.
In the dataflow graph, input signals are sources. As such, they don't have any predecessors.

## Synopsis
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