---
layout: default
title: EventSource
type_tag: class
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Event.h, url: 'reference/Event.h/'}
---
An event source extends the immutable `Events` interface with functions that support imperative input.

## Synopsis
{% highlight C++ %}
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
{% endhighlight %}

-----

<h1>Constructor <span class="type_tag">member function</span></h1>

Analogously defined to constructor of [Events](#events).

-----

<h1>Emit, operator() <span class="type_tag">member function</span></h1>

## Syntax
{% highlight C++ %}
void Emit(const E& e) const;        // (1)
void Emit(E&& e) const;             // (2)
void Emit() const;                  // (3)

void operator()(const E& e) const;  // (1)
void operator()(E&& e) const;       // (2)
void operator() const;              // (3)
{% endhighlight %}

## Semantics
Adds `e` to the queue of outgoing events of the linked event source node.

If `Emit` was called inside of a transaction function, it will return after the event has been queued and propagation is delayed until the transaction function returns.
Otherwise, propagation starts immediately and `Emit` blocks until it's done.

Using `operator()` is semantically equivalent to `Emit`.

(1) and (2) take an event value argument; (3) allows to omit that for token streams, where the emitted value is always `token`.

-----

<h1>operator<< <span class="type_tag">member function</span></h1>

## Syntax
{% highlight C++ %}
const EventSource& operator<<(const E& e) const;
const EventSource& operator<<(E&& e) const;
{% endhighlight %}

## Semantics
Semantically equivalent to `Emit`. In particular, chaining multiple events in a single statement will **not** execute them in a single transaction.