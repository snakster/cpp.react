---
layout: default
title: Signal.h
---
Contains the signal template classes and functions.

## Header
`#include "react/Signal.h"`

## Synopsis

### Classes
{% highlight C++ %}
namespace react
{
    // Signal
    class Signal<D,S>;

    // VarSignal
    class VarSignal<D,S>;

    // TempSignal
    class TempSignal<D,S,/*unspecified*/>;
}
{% endhighlight %}

### Functions
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

### Operators
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