### Header
`#include "react/Event.h"`

### Summary
Contains the event stream template classes and functions.

* [Constants](#constants)
  - [Token](#token)
* [Classes](classes)
  - [Events](#events)
  - [EventSource](#eventsource)
  - [TempEvents](#tempevents)
* [Functions](#functions)
  - [MakeEventSource](#makeeventsource)
  - [Merge](#merge)
  - [Filter](#filter)
  - [Transform](#transform)
  - [Flatten](#flatten)
  - [Tokenize](#tokenize)
* [Operators](#operators)

# Constants

## Token

This class is used as value type of token streams, which emit events without any value other than the fact that they occurred.

###### Definition
{% highlight C++ %}
namespace react
{
    enum class Token { value };
}
{% endhighlight %}

# Classes

## Events

An instance of this class acts as a proxy to an event stream node.
It takes shared ownership of the node, so while it exists, the node will not be destroyed.
Copy, move and assignment semantics are similar to `std::shared_ptr`.

###### Synopsis
{% highlight C++ %}
namespace react
{
    template
    <
        typename D,
        typename E = Token 
    >
    class Events
    {
    public:
        using ValueT = E;

        // Constructor
        Events();
        Events(const Events&);
        Events(Events&&);

        // Assignemnt
        Events& operator=(const Events&);
        Events& operator=(Events&& other);

        // Tests if two Events instances are equal
        bool Equals(const Events& other) const;

        // Tests if this instance is linked to a node
        bool IsValid() const;

        // Equivalent to react::Merge(*this, args ...)
        TempEvents<D,E,/*unspecified*/>
            Merge(const Events<D,TValues>& ... args) const;

        // Equivalent to react::Filter(*this, f)
        TempEvents<D,E,/*unspecified*/>
            Filter(F&& f) const;

        // Equivalent to react::Transform(*this, f)
        TempEvents<D,T,/*unspecified*/>
            Transform(F&& f) const;

        // Equivalent to react::Tokenize(*this)
        TempEvents<D,Token,/*unspecified*/>
            Tokenize() const;
    };
}
{% endhighlight %}
### Template parameters
<table>
<tr>
<td>E</td>
<td>Event value type. Aliased as member type <code>ValueT</code>. If this parameter is omitted, <code>Token</code> is used as the default.</td>
</tr>
</table>

### Member functions

#### (Constructor)
###### Syntax
{% highlight C++ %}
Events();                    // (1)
Events(const Events& other); // (2)
Events(Events&& other);      // (3)
{% endhighlight %}
###### Semantics
(1) Creates an invalid event stream that is not linked to an event node.

(2) Creates an event stream that links to the same event node as `other`.

(3) Creates an event stream that moves shared ownership of the event node from `other` to `this`.
As a result, `other` becomes invalid.

#### Equals
###### Syntax
{% highlight C++ %}
bool Equals(const Events& other) const;
{% endhighlight %}

###### Semantics
Returns true, if both `this` and `other` link to the same event node.
Since some reactive types use `==` as a combination operator, this function is used for unambiguous comparison.

#### IsValid
###### Syntax
{% highlight C++ %}
bool IsValid() const;
{% endhighlight %}

###### Semantics
Returns true, if `this` is linked to an event node.

#### Merge, Filter, Transform, Tokenize
###### Syntax
{% highlight C++ %}
template <typename ... TValues>
TempEvents<D,E,/*unspecified*/> Merge(const Events<D,TValues>& ... args) const;

template <typename F>
TempEvents<D,E,/*unspecified*/> Filter(F&& f) const;

template <typename F>
TempEvents<D,T,/*unspecified*/> Transform(F&& f) const;

TempEvents<D,Token,/*unspecified*/> Tokenize() const;
{% endhighlight %}

###### Semantics
Semantically equivalent to the respective free functions in namespace `react`.

## EventSource

An event source extends the immutable `Events` interface with functions that support imperative event input.

###### Synopsis
{% highlight C++ %}
namespace react
{
    template
    <
        typename D,
        typename E = Token
    >
    class EventSource : public Events<D,E>
    {
    public:
        // Constructor
        EventSource();
        EventSource(const EventSource&);
        EventSource(EventSource&&);

        // Assignemnt
        EventSource& operator=(const EventSource&);
        EventSource& operator=(EventSource&& other);

        // Emits an event
        void Emit(const E& e) const;
        void Emit(E&& e) const;
        void Emit() const;

        // Function operator version of `Emit`
        void operator()(const E& e) const;
        void operator()(E&& e) const;
        void operator()() const;

        // Stream operator version of `Emit`
        const EventSource& operator<<(const E& e) const;
        const EventSource& operator<<(E&& e) const;
    };
}
{% endhighlight %}

### Member functions

#### (Constructor)
Analogously defined to constructor of [Events](#events).

#### Emit
###### Syntax
{% highlight C++ %}
void Emit(const E& e) const;    // (1)
void Emit(E&& e) const;         // (2)

template <class = enable_if<is_same<E,Token>::value>::type>
void Emit() const;              // (3)
{% endhighlight %}

###### Semantics
Adds `e` to the queue of outgoing events of the linked event source node.

If `Emit` was called inside of a transaction function, it will return after the event has been queued and propagation is delayed until the transaction function returns.
Otherwise, propagation starts immediately and `Emit` blocks until it's done.

(1) and (2) take an event value argument; (3) allows to omit that for token streams, where the emitted value is always `token`.

#### Operator `()`
###### Syntax
{% highlight C++ %}
void operator()(const E& e) const;
void operator()(E&& e) const;

template <class = enable_if<is_same<E,Token>::value>::type>
void operator() const;
{% endhighlight %}

###### Semantics
Semantically equivalent to `Emit`.

#### Operator `<<`
###### Syntax
{% highlight C++ %}
const EventSource& operator<<(const E& e) const;
const EventSource& operator<<(E&& e) const;
{% endhighlight %}

###### Semantics
Semantically equivalent to `Emit`. In particular, chaining multiple events in a single statement will **not** execute them in a single propagation, unless it happens inside a transaction function.

## TempEvents

This class exposes additional type information of the linked node, which enables r-value based node merging at compile time.
`TempEvents` is usually not used as an l-value type, but instead implicitly converted to `Events`.

###### Synopsis
{% highlight C++ %}
namespace react
{
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
        TempEvents(const TempEvents&);
        TempEvents(TempEvents&&);

        // Assignemnt
        TempEvents& operator=(const TempEvents&);
        TempEvents& operator=(TempEvents&& other);
    };
}
{% endhighlight %}

### Member functions

#### (Constructor)
Analogously defined to constructor of [Events](#events).

# Functions

###### Synopsis
{% highlight C++ %}
namespace react
{
    // Creates a new event source
    EventSource<D,E>     MakeEventSource();
    EventSource<D,Token> MakeEventSource();

    // Creates a new event stream that merges events from given streams
    TempEvents<D,E,/*unspecified*/>
        Merge(const Events<D,E>& arg1, const Events<D,TValues>& ... args);

    // Creates a new event stream that filters events from other stream
    TempEvents<D,E,/*unspecified*/>
        Filter(const Events<D,E>& source, F&& func);
    TempEvents<D,E,/*unspecified*/>
        Filter(TempEvents<D,E,/*unspecified*/>&& source, F&& func);
    Events<D,E>
        Filter(const Events<D,E>& source,
               const SignalPack<D,TDepValues...>& depPack, FIn&& func);

    // Creates a new event stream that transforms events from other stream
    TempEvents<D,T,/*unspecified*/>
        Transform(const Events<D,E>& source, F&& func);
    TempEvents<D,T,/*unspecified*/>
        Transform(TempEvents<D,E,/*unspecified*/>&& source, F&& func);
    Events<D,T>
        Transform(const Events<D,TIn>& source,
                  const SignalPack<D,TDepValues...>& depPack, FIn&& func);

    // Creates a new event stream by flattening a signal of an event stream
    Events<D,T> Flatten(const Signal<D,Events<D,T>>& other);

    // Creates a new event stream by flattening a signal of an event stream
    Events<D,Token> Flatten(const Signal<D,Events<D,T>>& other);

    // Creates a token stream by transforming values from source to tokens
    TempEvents<D,Token,/*unspecified*/>
        Tokenize(TEvents&& source);
}
{% endhighlight %}

## MakeEventSource
###### Syntax
{% highlight C++ %}
template <typename D, typename E>
EventSource<D,E> MakeEventSource();     // (1)

template <typename D>
EventSource<D,Token> MakeEventSource(); // (2)
{% endhighlight %}

###### Semantics
Creates a new event source node and links it to the returned `EventSource` instance.

For (1), the event value type `E` has to be specified explicitly.
If it's omitted, (2) will create a token source.

###### Graph
<img src="images/flow_makesource.png" alt="Drawing" width="500px"/>

## Merge
###### Syntax
{% highlight C++ %}
template
<
    typename D,
    typename E,
    typename ... TValues
>
TempEvents<D,E,/*unspecified*/>
    Merge(const Events<D,E>& arg1, const Events<D,TValues>& ... args);
{% endhighlight %}

###### Semantics
Emit all events in `arg1, ... args`.

###### Graph
<img src="images/flow_merge.png" alt="Drawing" width="500px"/>

## Filter
###### Syntax
{% highlight C++ %}
// (1)
template
<
    typename D,
    typename E,
    typename F
>
TempEvents<D,E,/*unspecified*/>
    Filter(const Events<D,E>& source, F&& func);

// (2)
template
<
    typename D,
    typename E,
    typename F
>
TempEvents<D,E,/*unspecified*/>
    Filter(TempEvents<D,E,/*unspecified*/>&& source, F&& func);

// (3)
template
<
    typename D,
    typename E,
    typename ... TDepValues,
    typename F
>
Events<D,E> Filter(const Events<D,E,/*unspecified*/>& source,
                   const SignalPack<D,TDepValues...>& depPack, F&& func);
{% endhighlight %}

###### Semantics
For every event `e` in `source`, emit `e` if `func(e) == true`.

(1) Takes an l-value const reference.

(2) Takes an r-value reference. The linked node is combined with the new node.

(3) Similar to (1), but the synchronized values of signals in `depPack` are passed to `func` as additional arguments.

**Note**: Changes of signals in `depPack` do not trigger an update - only received events do.

The signature of `func` should be equivalent to:

* (1,2) `bool func(const E&)`
* ( 3 ) `bool func(const E&, const TDepValues& ...)`

###### Graph
(1) <br/>
<img src="images/flow_filter.png" alt="Drawing" width="500px"/>

(3) <br/>
<img src="images/flow_filter2.png" alt="Drawing" width="500px"/>

## Transform
###### Syntax
{% highlight C++ %}
// (1)
template
<
    typename D,
    typename E,
    typename F,
    typename T = result_of<F(E)>::type
>
TempEvents<D,T,/*unspecified*/>
    Transform(const Events<D,E>& source, F&& func);

// (2)
template
<
    typename D,
    typename E,
    typename F,
    typename T = result_of<F(E)>::type
>
TempEvents<D,T,/*unspecified*/>
    Transform(TempEvents<D,E,/*unspecified*/>&& source, F&& func);

// (3)
template
<
    typename D,
    typename E,
    typename F,
    typename ... TDepValues,
    typename T = result_of<F(E,TDepValues...)>::type
>
Events<D,T> Transform(const Events<D,E>& source,
                      const SignalPack<D,TDepValues...>& depPack, F&& func);
{% endhighlight %}

###### Semantics
For every event `e` in `source`, emit `t = func(e)`.

(1) Takes an l-value const reference.

(2) Takes an r-value reference. The linked node is combined with the new node.

(3) Similar to (1), but the synchronized values of signals in `depPack` are passed to `func` as additional arguments.

**Note**: Changes of signals in `depPack` do not trigger an update - only received events do.

The signature of `func` should be equivalent to:

* (1,2) `T func(const E&)`
* ( 3 ) `T func(const E&, const TDepValues& ...)`

###### Graph
(1) <br/>
<img src="images/flow_transform.png" alt="Drawing" width="500px"/>

(3) <br/>
<img src="images/flow_transform2.png" alt="Drawing" width="500px"/>

## Flatten
###### Syntax
{% highlight C++ %}
template
<
    typename D,
    typename T
>
Events<D,T> Flatten(const Signal<D,Events<D,T>>& other);
{% endhighlight %}

###### Semantics
TODO

###### Graph
<img src="images/flow_eventflatten.png" alt="Drawing" width="500px"/>


# Operators

###### Synopsis
{% highlight C++ %}
namespace react
{
    // Merge
    TempEvents<D,E,/*unspecified*/>
        operator|(const TLeftEvents& lhs,
                  const TRightEvents& rhs);
    TempEvents<D,E,/*unspecified*/>
        operator|(TempEvents<D,TLeftVal,/*unspecified*/>&& lhs,
                  TempEvents<D,TRightVal,/*unspecified*/>&& rhs);
    TempEvents<D,E,/*unspecified*/>
        operator|(TempEvents<D,TLeftVal,/*unspecified*/>&& lhs,
                  const TRightEvents& rhs);
    TempEvents<D,E,/*unspecified*/>
        operator|(const TLeftEvents& lhs,
                  TempEvents<D,TRightVal,/*unspecified*/>&& rhs);
}
{% endhighlight %}