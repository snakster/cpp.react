---
layout: default
---
### Header
`#include "react/Domain.h"`

### Summary
Contains the base class for reactive domains and the macro to define them.

* [Classes](#classes)
  - [DomainBase](#domainbase)
  - [Continuation](#continuation)
* [Functions](#functions)
  - [MakeContinuation](#makecontinuation)
* [Macros](#macros)
  - [REACTIVE_DOMAIN](#reactive_domain)
  - [USING_REACTIVE_DOMAIN](#using_reactive_domain)

# Classes

# Functions
###### Synopsis
``` C++
namespace react
{
    // Creates a continuation from domain D to D2
    Continuation<D,D2>
        MakeContinuation<D,D2>(const Signal<D,S>& trigger, F&& func);
    Continuation<D,D2>
        MakeContinuation<D,D2>(const Events<D,E>& trigger, F&& func);
    Continuation<D,D2>
        MakeContinuation<D,D2>(const Events<D,E>& trigger,
                               const SignalPack<D,TDepValues...>& depPack, F&& func);
}
```

## MakeContinuation
###### Syntax
``` C++
// (1)
template
<
    typename D,
    typename D2 = D,
    typename S,
    typename FIn
>
Continuation<D,D2> MakeContinuation(const Signal<D,S>& trigger, FIn&& func);

// (2)
template
<
    typename D,
    typename D2 = D,
    typename E,
    typename FIn
>
Continuation<D,D2> MakeContinuation(const Events<D,E>& trigger, FIn&& func);

// (3)
template
<
    typename D,
    typename D2 = D,
    typename E,
    typename FIn,
    typename ... TDepValues
>
Continuation<D,D2>
    MakeContinuation(const Events<D,E>& trigger,
                     const SignalPack<D,TDepValues...>& depPack, FIn&& func);
```

###### Semantics
(1) When the signal value `s` of `trigger` changes, `func(s)` is executed in a transaction of domain `D2`.
In pseudo code:
``` C++
D2::DoTransaction([func, s] {
    func(s)
});
```

(2) For every event `e` in `trigger`, `func(e)` is called in a transaction of domain `D2`.
Multiple events from the same turn are captured in a single transaction.
In pseudo code:
``` C++
D2::DoTransaction([func, events] {
    for (const auto& e : events)
        func(e);
});
```

(3) Similar to (2), but the synchronized values of signals in `depPack` are passed to `func` as additional arguments.
Changes of signals in `depPack` do not trigger an update - only received events do.
In pseudo code:
``` C++
D2::DoTransaction([func, events, depValues...] {
    for (const auto& e : events)
        func(e, depValues ...);
});
```

The signature of `func` should be equivalent to:

* (1) `void func(const S&)`
* (2) `void func(const E&)`
* (3) `void func(const E&, const TDepValues& ...)`

``` C++
D2::DoTransaction([func, events, depValues...] {
    for (const auto& e : events)
        func(e, depValues ...);
});
```

# Macros

## REACTIVE_DOMAIN
###### Syntax
``` C++
REACTIVE_DOMAIN(name, ...)
```

###### Semantics
Defines a reactive domain `name`, declared as `class name : public DomainBase<name>`. Also initializes static data for this domain.

The optional parameter is the propagation engine. If omitted, `Toposort<sequential>` is used as default.

## USING_REACTIVE_DOMAIN
###### Syntax
``` C++
USING_REACTIVE_DOMAIN(name)
```

###### Semantics
Defines the following type aliases for the given domain name in the current scope:
``` C++
template <typename S>
using SignalT = Signal<name,S>;

template <typename S>
using VarSignalT = VarSignal<name,S>;

template <typename E = Token>
using EventsT = Events<name,E>;

template <typename E = Token>
using EventSourceT = EventSource<name,E>;

using ObserverT = Observer<name>;

using ReactorT = Reactor<name>;
```