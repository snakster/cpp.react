---
layout: default
title: Signal.h
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
---
Contains the signal template classes and functions.

## Header
{% highlight C++ %}
#include "react/Signal.h"
{% endhighlight %}

## Synopsis

### Classes

<div class="highlight"><pre><code class="c++"><span class="k">namespace</span> <span class="n">react</span>
<span class="p">{</span>
    <span class="c1">// Signal</span>
    <span class="k">class</span> <a class="code_link" href="{{ site.baseurl }}/reference/Signal.h/Signal.html">Signal</a><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">S</span><span class="o">&gt;</span>

    <span class="c1">// VarSignal</span>
    <span class="k">class</span> <a class="code_link" href="{{ site.baseurl }}/reference/Signal.h/VarSignal.html">VarSignal</a><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">S</span><span class="o">&gt;</span>

    <span class="c1">// TempSignal</span>
    <span class="k">class</span> <a class="code_link" href="{{ site.baseurl }}/reference/Signal.h/TempSignal.html">TempSignal</a><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">S</span><span class="p">,</span><span class="cm">/*unspecified*/</span><span class="o">&gt;</span>
<span class="p">}</span>
</code></pre></div>

<!--
{% highlight C++ %}
namespace react
{
    // Signal
    class Signal<D,S>

    // VarSignal
    class VarSignal<D,S>

    // TempSignal
    class TempSignal<D,S,/*unspecified*/>
}
{% endhighlight %}
-->

### Functions

<div class="highlight"><pre><code class="c++"><span class="k">namespace</span> <span class="n">react</span>
<span class="p">{</span>
    <span class="c1">//Creates a new variable signal</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Signal.h/MakeVar.html">MakeVar</a><span class="p">(</span><span class="n">V</span><span class="o">&amp;&amp;</span> <span class="n">init</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">VarSignal</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">S</span><span class="o">&gt;</span>

    <span class="c1">// Creates a signal as a function of other signals</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Signal.h/MakeSignal.html">MakeSignal</a><span class="p">(</span><span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">,</span> <span class="k">const</span> <span class="n">Signal</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">TValues</span><span class="o">&gt;&amp;</span> <span class="p">...</span> <span class="n">args</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">TempSignal</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">S</span><span class="p">,</span><span class="cm">/*unspecified*/</span><span class="o">&gt;</span>

    <span class="c1">// Creates a new signal by flattening a signal of a signal</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Signal.h/Flatten.html">Flatten</a><span class="p">(</span><span class="k">const</span> <span class="n">Signal</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">Signal</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">T</span><span class="o">&gt;&gt;&amp;</span> <span class="n">other</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">Signal</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">T</span><span class="o">&gt;</span>
<span class="p">}</span> 
</code></pre></div>

<!--
{% highlight C++ %}
namespace react
{
    //Creates a new variable signal
    MakeVar(V&& init)
      -> VarSignal<D,S>

    // Creates a signal as a function of other signals
    MakeSignal(F&& func, const Signal<D,TValues>& ... args)
      -> TempSignal<D,S,/*unspecified*/>

    // Creates a new signal by flattening a signal of a signal
    Flatten(const Signal<D,Signal<D,T>>& other)
      -> Signal<D,T>
} 
{% endhighlight %}
-->

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
    OP(const TSignal& arg)
      -> TempSignal<D,S,/*unspecified*/>

    // OP <TempSignal>
    OP(TempSignal<D,TVal,/*unspecified*/>&& arg)
      -> TempSignal<D,S,/*unspecified*/>

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
    BIN_OP(const TLeftSignal& lhs, const TRightSignal& rhs)
      -> TempSignal<D,S,/*unspecified*/>

    // <Signal> BIN_OP <NonSignal>
    BIN_OP(const TLeftSignal& lhs, TRightVal&& rhs)
      -> TempSignal<D,S,/*unspecified*/>

    // <NonSignal> BIN_OP <Signal>
    BIN_OP(TLeftVal&& lhs, const TRightSignal& rhs)
      -> TempSignal<D,S,/*unspecified*/>

    // <TempSignal> BIN_OP <TempSignal>
    BIN_OP(TempSignal<D,TLeftVal,/*unspecified*/>&& lhs,
           TempSignal<D,TRightVal,/*unspecified*/>&& rhs)
      -> TempSignal<D,S,/*unspecified*/>

    // <TempSignal> BIN_OP <Signal>
    BIN_OP(TempSignal<D,TLeftVal,/*unspecified*/>&& lhs,
           const TRightSignal& rhs)
      -> TempSignal<D,S,/*unspecified*/>

    // <Signal> BIN_OP <TempSignal>
    BIN_OP(const TLeftSignal& lhs,
           TempSignal<D,TRightVal,/*unspecified*/>&& rhs)
      -> TempSignal<D,S,/*unspecified*/>

    // <TempSignal> BIN_OP <NonSignal>
    BIN_OP(TempSignal<D,TLeftVal,/*unspecified*/>&& lhs,
           TRightVal&& rhs)
      -> TempSignal<D,S,/*unspecified*/>

    // <NonSignal> BIN_OP <TempSignal>
    BIN_OP(TLeftVal&& lhs,
           TempSignal<D,TRightVal,/*unspecified*/>&& rhs)
      -> TempSignal<D,S,/*unspecified*/>
}
{% endhighlight %}