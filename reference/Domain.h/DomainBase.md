---
layout: default
---
# DomainBase

This class contains the functionality for domains defined with `REACTIVE_DOMAIN`.
A domain cannot be instantiated. It is used to group domain-specific type aliases and static functions.

## Synopsis
{% highlight C++ %}
namespace react
{
    template <typename D>
    class DomainBase
    {
    public:
        DomainBase() = delete;

        // Type aliases for this domain
        template <typename S>
        using SignalT = Signal<D,S>;

        template <typename S>
        using VarSignalT = VarSignal<D,S>;

        template <typename E = Token>
        using EventsT = Events<D,E>;

        template <typename E = Token>
        using EventSourceT = EventSource<D,E>;

        using ObserverT = Observer<D>;

        using ReactorT = Reactor<D>;

        // Run given function in transaction mode
        static void DoTransaction(F&& func);
        static void DoTransaction(TurnFlagsT flags, F&& func);
    };
}
{% endhighlight %}

## Member functions

### (Constructor)
#### Syntax
``` C++
DomainBase() = delete;
```

#### Semantics
Deleted default constructor to prevent instantiation.

### DoTransaction
#### Syntax
``` C++
template <typename F>
static void DoTransaction(F&& func);                    // (1)

template <typename F>
static void DoTransaction(TurnFlagsT flags, F&& func);  // (2)
```

#### Semantics
Runs `func` in transaction mode. During transaction mode, all reactive inputs are collected and propagated in a single turn after `func` returns.

The purpose of this is
* to ensure consistency, if there exist certain invariants between reactive inputs;
* to avoid redundant re-calculations;
* to avoid the overhead of additional turns.

(1) uses the default turn flags for the transaction. (2) allows to set flags explicitly.