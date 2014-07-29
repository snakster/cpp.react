---
layout: default
title: Domain.h
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
---
Contains the base class for reactive domains and the macro to define them.

## Header
{% highlight C++ %}
#include "react/Domain.h"
{% endhighlight %}

## Synopsis

### Constants
{% highlight C++ %}
namespace react
{
    // Domain concurrency policies
    enum EDomainMode
    {
        sequential,
        sequential_concurrent,
        parallel,
        parallel_concurrent
    }

    // Optional transaction flags
    enum ETransactionFlags
    {
        allow_merging
    }
}
{% endhighlight %}

### Classes

<div class="highlight"><pre><code class="c++"><span class="k">namespace</span> <span class="n">react</span>
<span class="p">{</span>
    <span class="c1">// WeightHint</span>
    <span class="k">enum</span> <span class="k">class</span> <a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/WeightHint.html">WeightHint</a>

    <span class="c1">// Proxy to continuation node</span>
    <span class="k">class</span> <a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/Continuation.html">Continuation</a><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">D2</span><span class="o">&gt;</span>

    <span class="c1">// Status handle to wait on asynchrounous transactions</span>
    <span class="k">class</span> <a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/TransactionStatus.html">TransactionStatus</a>

    <span class="c1">// Supplementary type for engine declarations</span>
    <span class="k">enum</span> EPropagationMode

    <span class="c1">// Toposort engine class template</span>
    <span class="k">class</span> ToposortEngine<span class="o">&lt;</span><span class="n">EPropagationMode m</span><span class="o">&gt;</span>

    <span class="c1">// Pulsecount engine class template</span>
    <span class="k">class</span> PulsecountEngine<span class="o">&lt;</span><span class="n">EPropagationMode m</span><span class="o">&gt;</span>

    <span class="c1">// Subtree engine class template</span>
    <span class="k">class</span> SubtreeEngine<span class="o">&lt;</span><span class="n">EPropagationMode m</span><span class="o">&gt;</span>
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
    <span class="c1">// Bitmask of ETransactionFlags</span>
    <span class="k">using</span> <span class="n">TransactionFlagsT</span> <span class="o">=</span> <span class="n">underlying_type</span><span class="o">&lt;</span><span class="n">ETransactionFlags</span><span class="o">&gt;::</span><span class="n">type</span><span class="p">;</span>

    <span class="c1">// Executes given function as transaction (blocking)</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/DoTransaction.html">DoTransaction</a><span class="o">&lt;</span><span class="n">D</span><span class="o">&gt;</span><span class="p">(</span><span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span> <span class="o">-&gt;</span> <span class="kt">void</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/DoTransaction.html">DoTransaction</a><span class="o">&lt;</span><span class="n">D</span><span class="o">&gt;</span><span class="p">(</span><span class="n">TransactionFlagsT</span> <span class="n">flags</span><span class="p">,</span> <span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span> <span class="o">-&gt;</span> <span class="kt">void</span>

    <span class="c1">// Enqueues given function as as asynchronous transaction (non-blocking)</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/AsyncTransaction.html">AsyncTransaction</a><span class="o">&lt;</span><span class="n">D</span><span class="o">&gt;</span><span class="p">(</span><span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span> <span class="o">-&gt;</span> <span class="kt">void</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/AsyncTransaction.html">AsyncTransaction</a><span class="o">&lt;</span><span class="n">D</span><span class="o">&gt;</span><span class="p">(</span><span class="n">TransactionStatus</span><span class="o">&amp;</span> <span class="n">status</span><span class="p">,</span> <span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span> <span class="o">-&gt;</span> <span class="kt">void</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/AsyncTransaction.html">AsyncTransaction</a><span class="o">&lt;</span><span class="n">D</span><span class="o">&gt;</span><span class="p">(</span><span class="n">TransactionFlagsT</span> <span class="n">flags</span><span class="p">,</span> <span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span> <span class="o">-&gt;</span> <span class="kt">void</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/AsyncTransaction.html">AsyncTransaction</a><span class="o">&lt;</span><span class="n">D</span><span class="o">&gt;</span><span class="p">(</span><span class="n">TransactionFlagsT</span> <span class="n">flags</span><span class="p">,</span> <span class="n">TransactionStatus</span><span class="o">&amp;</span> <span class="n">status</span><span class="p">,</span> <span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span> <span class="o">-&gt;</span> <span class="kt">void</span>

    <span class="c1">// Creates a continuation from domain D to D2</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/MakeContinuation.html">MakeContinuation</a><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">D2</span><span class="o">&gt;</span><span class="p">(</span><span class="k">const</span> <span class="n">Signal</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">S</span><span class="o">&gt;&amp;</span> <span class="n">trigger</span><span class="p">,</span> <span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">Continuation</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">D2</span><span class="o">&gt;</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/MakeContinuation.html">MakeContinuation</a><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">D2</span><span class="o">&gt;</span><span class="p">(</span><span class="k">const</span> <span class="n">Events</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">E</span><span class="o">&gt;&amp;</span> <span class="n">trigger</span><span class="p">,</span> <span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">Continuation</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">D2</span><span class="o">&gt;</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/MakeContinuation.html">MakeContinuation</a><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">D2</span><span class="o">&gt;</span><span class="p">(</span><span class="k">const</span> <span class="n">Events</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">E</span><span class="o">&gt;&amp;</span> <span class="n">trigger</span><span class="p">,</span> <span class="k">const</span> <span class="n">SignalPack</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">TDepValues</span><span class="p">...</span><span class="o">&gt;&amp;</span> <span class="n">depPack</span><span class="p">,</span> <span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">Continuation</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">D2</span><span class="o">&gt;</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/MakeContinuation.html">MakeContinuation</a><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">D2</span><span class="o">&gt;</span><span class="p">(</span><span class="n">TransactionFlagsT</span> <span class="n">flags</span><span class="p">,</span> <span class="k">const</span> <span class="n">Signal</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">S</span><span class="o">&gt;&amp;</span> <span class="n">trigger</span><span class="p">,</span> <span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">Continuation</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">D2</span><span class="o">&gt;</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/MakeContinuation.html">MakeContinuation</a><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">D2</span><span class="o">&gt;</span><span class="p">(</span><span class="n">TransactionFlagsT</span> <span class="n">flags</span><span class="p">,</span> </span><span class="k">const</span> <span class="n">Events</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">E</span><span class="o">&gt;&amp;</span> <span class="n">trigger</span><span class="p">,</span> <span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">Continuation</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">D2</span><span class="o">&gt;</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/MakeContinuation.html">MakeContinuation</a><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">D2</span><span class="o">&gt;</span><span class="p">(</span><span class="n">TransactionFlagsT</span> <span class="n">flags</span><span class="p">,</span> <span class="k">const</span> <span class="n">Events</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">E</span><span class="o">&gt;&amp;</span> <span class="n">trigger</span><span class="p">,</span> <span class="k">const</span> <span class="n">SignalPack</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">TDepValues</span><span class="p">...</span><span class="o">&gt;&amp;</span> <span class="n">depPack</span><span class="p">,</span> <span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">Continuation</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">D2</span><span class="o">&gt;</span>
<span class="p">}</span>
</code></pre></div>

<!--
{% highlight C++ %}
namespace react
{
    using TransactionFlagsT = underlying_type<ETurnFlags>::type;

    // DoTransaction
    DoTransaction<D>(F&& func) -> void
    DoTransaction<D>(TransactionFlagsT flags, F&& func) -> void

    // AsyncTransaction
    AsyncTransaction<D>(F&& func) -> void
    AsyncTransaction<D>(TransactionFlagsT flags, F&& func) -> void
    AsyncTransaction<D>(TransactionStatus& status, F&& func) -> void
    AsyncTransaction<D>(TransactionFlagsT flags, TransactionStatus& status, F&& func) -> void

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

<div class="highlight"><pre><code class="c++"><span class="c1">// Defines a reactive domain</span>
<a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/REACTIVE_DOMAIN.html">REACTIVE_DOMAIN</a><span class="p">(</span><span class="n">name</span><span class="p">,</span> <span class="n">EDomainMode</span> <span class="n">mode</span><span class="p">)</span>
<a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/REACTIVE_DOMAIN.html">REACTIVE_DOMAIN</a><span class="p">(</span><span class="n">name</span><span class="p">,</span> <span class="n">EDomainMode</span> <span class="n">mode</span><span class="p">,</span> <span class="n">engine</span><span class="p">)</span>

<span class="c1">// Defines type aliases for the given reactive domain</span>
<a class="code_link" href="{{ site.baseurl }}/reference/Domain.h/USING_REACTIVE_DOMAIN.html">USING_REACTIVE_DOMAIN</a><span class="p">(</span><span class="n">name</span><span class="p">)</span>
</code></pre></div>

<!--
{% highlight C++ %}
REACTIVE_DOMAIN(name, EDomainMode mode)
REACTIVE_DOMAIN(name, EDomainMode mode, engine)

USING_REACTIVE_DOMAIN(name)
{% endhighlight %}
-->