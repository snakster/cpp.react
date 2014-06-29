### Header
`#include "react/Algorithm.h"`

### Summary
Contains various reactive operations to combine and convert signals and events.

* [Functions](#functions)
  - [Iterate](#iterate)
  - [Hold](#hold)
  - [Snapshot](#snapshot)
  - [Monitor](#monitor)
  - [Pulse](#pulse)
  - [OnChanged](#onchanged)
  - [OnChangedTo](#onchangedto)

# Functions
###### Synopsis
``` C++
namespace react
{
    // Iteratively combines signal value with values from event stream
    Signal<D,S> Iterate(const Events<D,E>& events, V&& init, F&& func);
    Signal<D,S> Iterate(const Events<D,E>& events, V&& init,
                        const SignalPack<D,TDepValues...>& depPack, FIn&& func);

    // Hold the most recent event in a signal
    Signal<D,T> Hold(const Events<D,T>& events, V&& init);

    // Sets signal value to value of target when event is received
    Signal<D,S> Snapshot(const Events<D,E>& trigger, const Signal<D,S>& target);

    // Emits value changes of target signal
    Events<D,S> Monitor(const Signal<D,S>& target);

    // Emits value of target signal when event is received
    Events<D,S> Pulse(const Events<D,E>& trigger, const Signal<D,S>& target);

    // Emits token when target signal was changed
    Events<D,Token> OnChanged(const Signal<D,S>& target);

    // Emits token when target signal was changed to value
    Events<D,Token> OnChangedTo(const Signal<D,S>& target, V&& value);
}
```
## Iterate
###### Syntax
``` C++
// (1)
template
<
    typename D,
    typename E,
    typename V,
    typename F,
    typename S = decay<V>::type
>
Signal<D,S> Iterate(const Events<D,E>& events, V&& init, F&& func); 

// (2)
template
<
    typename D,
    typename E,
    typename V,
    typename F,
    typename ... TDepValues,
    typename S = decay<V>::type
>
Signal<D,S> Iterate(const Events<D,E>& events, V&& init,
                    const SignalPack<D,TDepValues...>& depPack, F&& func); 
```
###### Semantics
(1) Creates a signal with an initial value `v = init`.

- If the return type of `func` is `S`: For every received event `e` in `events`, `v` is updated to `v = func(e,v)`.

- If the return type of `func` is `void`: For every received event `e` in `events`, `v` is passed by non-cost reference to `func(e,v)`, making it mutable.
This variant can be used if copying and comparing `S` is prohibitively expensive.
Because the old and new values cannot be compared, updates will always trigger a change.

(2) Similar to (1), but the synchronized values of signals in `depPack` are passed to `func` as additional arguments. Changes of signals in `depPack` do _not_ trigger an update - only received events do.

The signature of `func` should be equivalent to:

* (1a) `S func(const E&, const S&)`
* (1b) `void func(const E&, S&)`
* (2a) `S func(const E&, const S&, const TDepValues& ...)`
* (2b) `void func(const E&, S&, const TDepValues& ...)`

###### Graph
(1a) <br/>
<img src="images/flow_iterate.png" alt="Drawing" width="500px"/>

(2a) <br/>
<img src="images/flow_iterate2.png" alt="Drawing" width="500px"/>

## Hold
###### Syntax
``` C++
template
<
    typename D,
    typename V,
    typename T = decay<V>::type
>
Signal<D,T> Hold(const Events<D,T>& events, V&& init);
```

###### Semantics
Creates a signal with an initial value `v = init`.
For received event values `e1, e2, ... eN` in `events`, it is updated to `v = eN`.

## Snapshot
###### Syntax
``` C++
template
<
    typename D,
    typename S,
    typename E
>
Signal<D,S> Snapshot(const Events<D,E>& trigger, const Signal<D,S>& target);
```

###### Graph
<img src="images/flow_snapshot.png" alt="Drawing" width="500px"/>

###### Semantics
Creates a signal with value `v = target.Value()`.
The value is set on construction and updated **only** when receiving an event from `trigger`.

## Monitor
###### Syntax
``` C++
template
<
    typename D,
    typename S
>
Events<D,S> Monitor(const Signal<D,S>& target);
```

###### Semantics
When `target` changes, emit the new value `e = target.Value()`.

###### Graph
<img src="images/flow_monitor.png" alt="Drawing" width="500px"/>

## Pulse
###### Syntax
``` C++
template
<
    typename D,
    typename S,
    typename E
>
Events<D,S> Pulse(const Events<D,E>& trigger, const Signal<D,S>& target);
```

###### Semantics
Creates an event stream that emits `target.Value()` when receiving an event from `trigger`
The values of the received events are irrelevant.

###### Graph
<img src="images/flow_pulse.png" alt="Drawing" width="500px"/>

## OnChanged
###### Syntax
``` C++
template
<
    typename D,
    typename S
>
Events<D,Token> OnChangedTo(const Signal<D,S>& target);
```

###### Semantics
Creates a token stream that emits when `target` is changed.

## OnChangedTo
###### Syntax
``` C++
template
<
    typename D,
    typename V,
    typename S = decay<V>::type
>
Events<D,Token> OnChangedTo(const Signal<D,S>& target, V&& value);
```

###### Parameters
Creates a token stream that emits when `target` is changed and `target.Value() == value`.

`V` and `S` should be comparable with `==`.