---
layout: default
title: Domain.h
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
---
Contains the base class for reactive domains and the macro to define them.

## Header
`#include "react/Domain.h"`

## Synopsis

### Constants
{% highlight C++ %}
namespace react
{
    enum EDomainMode
    {
        sequential,
        sequential_concurrent,
        parallel,
        parallel_concurrent
    };

    enum ETurnFlags
    {
        allow_merging
    };

    enum EInputMode;

    enum EPropagationMode;
}
{% endhighlight %}

### Classes

<div class="highlight"><pre><code class="c++"><span class="k">namespace</span> <span class="n">react</span>
<span class="p">{</span>
    <span class="c1">// DomainBase</span>
    <span class="k">class</span> <a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/DomainBase.html">DomainBase</a><span class="o">&lt;</span><span class="n">D</span><span class="o">&gt;</span>

    <span class="c1">// Continuation</span>
    <span class="k">class</span> <a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/Continuation.html">Continuation</a><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">D2</span><span class="o">&gt;</span>

    <span class="c1">// TransactionStatus</span>
    <span class="k">class</span> <a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/TransactionStatus.html">TransactionStatus</a>
<span class="p">}</span>
</code></pre></div>

<!--
{% highlight C++ %}
namespace react
{
    // DomainBase
    class DomainBase<D>

    // Continuation
    class Continuation<D,D2>

    // TransactionStatus
    class TransactionStatus
}
{% endhighlight %}
-->

### Functions

<div class="highlight"><pre><code class="c++"><span class="k">namespace</span> <span class="n">react</span>
<span class="p">{</span>
    <span class="k">using</span> <span class="n">TurnFlagsT</span> <span class="o">=</span> <span class="n">underlying_type</span><span class="o">&lt;</span><span class="n">ETurnFlags</span><span class="o">&gt;::</span><span class="n">type</span><span class="p">;</span>

    <span class="c1">// DoTransaction</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/DoTransaction.html">DoTransaction</a><span class="o">&lt;</span><span class="n">D</span><span class="o">&gt;</span><span class="p">(</span><span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span> <span class="o">-&gt;</span> <span class="kt">void</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/DoTransaction.html">DoTransaction</a><span class="o">&lt;</span><span class="n">D</span><span class="o">&gt;</span><span class="p">(</span><span class="n">TurnFlagsT</span> <span class="n">flags</span><span class="p">,</span> <span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span> <span class="o">-&gt;</span> <span class="kt">void</span>

    <span class="c1">// AsyncTransaction</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/AsyncTransaction.html">AsyncTransaction</a><span class="o">&lt;</span><span class="n">D</span><span class="o">&gt;</span><span class="p">(</span><span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span> <span class="o">-&gt;</span> <span class="kt">void</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/AsyncTransaction.html">AsyncTransaction</a><span class="o">&lt;</span><span class="n">D</span><span class="o">&gt;</span><span class="p">(</span><span class="n">TurnFlagsT</span> <span class="n">flags</span><span class="p">,</span> <span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span> <span class="o">-&gt;</span> <span class="kt">void</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/AsyncTransaction.html">AsyncTransaction</a><span class="o">&lt;</span><span class="n">D</span><span class="o">&gt;</span><span class="p">(</span><span class="n">TransactionStatus</span><span class="o">&amp;</span> <span class="n">status</span><span class="p">,</span> <span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span> <span class="o">-&gt;</span> <span class="kt">void</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/AsyncTransaction.html">AsyncTransaction</a><span class="o">&lt;</span><span class="n">D</span><span class="o">&gt;</span><span class="p">(</span><span class="n">TurnFlagsT</span> <span class="n">flags</span><span class="p">,</span> <span class="n">TransactionStatus</span><span class="o">&amp;</span> <span class="n">status</span><span class="p">,</span> <span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span> <span class="o">-&gt;</span> <span class="kt">void</span>

    <span class="c1">// Creates a continuation from domain D to D2</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/MakeContinuation.html">MakeContinuation</a><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">D2</span><span class="o">&gt;</span><span class="p">(</span><span class="k">const</span> <span class="n">Signal</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">S</span><span class="o">&gt;&amp;</span> <span class="n">trigger</span><span class="p">,</span> <span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">Continuation</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">D2</span><span class="o">&gt;</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/MakeContinuation.html">MakeContinuation</a><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">D2</span><span class="o">&gt;</span><span class="p">(</span><span class="k">const</span> <span class="n">Events</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">E</span><span class="o">&gt;&amp;</span> <span class="n">trigger</span><span class="p">,</span> <span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">Continuation</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">D2</span><span class="o">&gt;</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/MakeContinuation.html">MakeContinuation</a><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">D2</span><span class="o">&gt;</span><span class="p">(</span><span class="k">const</span> <span class="n">Events</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">E</span><span class="o">&gt;&amp;</span> <span class="n">trigger</span><span class="p">,</span>
                           <span class="k">const</span> <span class="n">SignalPack</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">TDepValues</span><span class="p">...</span><span class="o">&gt;&amp;</span> <span class="n">depPack</span><span class="p">,</span> <span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">Continuation</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">D2</span><span class="o">&gt;</span>
<span class="p">}</span>
</code></pre></div>

<!--
{% highlight C++ %}
namespace react
{
    using TurnFlagsT = underlying_type<ETurnFlags>::type;

    // DoTransaction
    DoTransaction<D>(F&& func) -> void
    DoTransaction<D>(TurnFlagsT flags, F&& func) -> void

    // AsyncTransaction
    AsyncTransaction<D>(F&& func) -> void
    AsyncTransaction<D>(TurnFlagsT flags, F&& func) -> void
    AsyncTransaction<D>(TransactionStatus& status, F&& func) -> void
    AsyncTransaction<D>(TurnFlagsT flags, TransactionStatus& status, F&& func) -> void

    // Creates a continuation from domain D to D2
    MakeContinuation<D,D2>(const Signal<D,S>& trigger, F&& func)
      -> Continuation<D,D2>
    MakeContinuation<D,D2>(const Events<D,E>& trigger, F&& func)
      -> Continuation<D,D2>
    MakeContinuation<D,D2>(const Events<D,E>& trigger,
                           const SignalPack<D,TDepValues...>& depPack, F&& func)
      -> Continuation<D,D2>
}
{% endhighlight %}
-->

### Macros
{% highlight C++ %}
#define REACTIVE_DOMAIN(name, ...)

#define USING_REACTIVE_DOMAIN(name)
{% endhighlight %}