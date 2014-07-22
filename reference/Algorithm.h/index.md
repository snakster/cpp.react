---
layout: default
title: Algorithm.h
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
---
Contains various reactive operations to combine and convert signals and events.

## Header
{% highlight C++ %}
#include "react/Algorithm.h"
{% endhighlight %}

## Synopsis

### Functions

<div class="highlight"><pre><code class="c++"><span class="k">namespace</span> <span class="n">react</span>
<span class="p">{</span>
    <span class="c1">// Holds most recent event in a signal</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Algorithm.h/Hold.html">Hold</a><span class="p">(</span><span class="k">const</span> <span class="n">Events</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">T</span><span class="o">&gt;&amp;</span> <span class="n">events</span><span class="p">,</span> <span class="n">V</span><span class="o">&amp;&amp;</span> <span class="n">init</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">Signal</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">T</span><span class="o">&gt;</span>

    <span class="c1">// Emits value changes of signal as events</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Algorithm.h/Monitor.html">Monitor</a><span class="p">(</span><span class="k">const</span> <span class="n">Signal</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">S</span><span class="o">&gt;&amp;</span> <span class="n">target</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">Events</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">S</span><span class="o">&gt;</span>

    <span class="c1">// Folds values from event stream into a signal</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Algorithm.h/Iterate.html">Iterate</a><span class="p">(</span><span class="k">const</span> <span class="n">Events</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">E</span><span class="o">&gt;&amp;</span> <span class="n">events</span><span class="p">,</span> <span class="n">V</span><span class="o">&amp;&amp;</span> <span class="n">init</span><span class="p">,</span> <span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">Signal</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">S</span><span class="o">&gt;</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Algorithm.h/Iterate.html">Iterate</a><span class="p">(</span><span class="k">const</span> <span class="n">Events</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">E</span><span class="o">&gt;&amp;</span> <span class="n">events</span><span class="p">,</span> <span class="n">V</span><span class="o">&amp;&amp;</span> <span class="n">init</span><span class="p">,</span>
            <span class="k">const</span> <span class="n">SignalPack</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">TDepValues</span><span class="p">...</span><span class="o">&gt;&amp;</span> <span class="n">depPack</span><span class="p">,</span> <span class="n">FIn</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">Signal</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">S</span><span class="o">&gt;</span>

    <span class="c1">// Sets signal value to value of target signal when event is received</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Algorithm.h/Snapshot.html">Snapshot</a><span class="p">(</span><span class="k">const</span> <span class="n">Events</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">E</span><span class="o">&gt;&amp;</span> <span class="n">trigger</span><span class="p">,</span> <span class="k">const</span> <span class="n">Signal</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">S</span><span class="o">&gt;&amp;</span> <span class="n">target</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">Signal</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">S</span><span class="o">&gt;</span>

    <span class="c1">// Emits value of target signal when event is received</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Algorithm.h/Pulse.html">Pulse</a><span class="p">(</span><span class="k">const</span> <span class="n">Events</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">E</span><span class="o">&gt;&amp;</span> <span class="n">trigger</span><span class="p">,</span> <span class="k">const</span> <span class="n">Signal</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">S</span><span class="o">&gt;&amp;</span> <span class="n">target</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">Events</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">S</span><span class="o">&gt;</span>

    <span class="c1">// Emits token when target signal was changed</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Algorithm.h/OnChanged.html">OnChanged</a><span class="p">(</span><span class="k">const</span> <span class="n">Signal</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">S</span><span class="o">&gt;&amp;</span> <span class="n">target</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">Events</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">Token</span><span class="o">&gt;</span>

    <span class="c1">// Emits token when target signal was changed to value</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Algorithm.h/OnChangedTo.html">OnChangedTo</a><span class="p">(</span><span class="k">const</span> <span class="n">Signal</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">S</span><span class="o">&gt;&amp;</span> <span class="n">target</span><span class="p">,</span> <span class="n">V</span><span class="o">&amp;&amp;</span> <span class="n">value</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">Events</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">Token</span><span class="o">&gt;</span>
<span class="p">}</span>
</code></pre></div>

<!--
namespace react
{
    // Iteratively combines signal value with values from event stream
    Iterate(const Events<D,E>& events, V&& init, F&& func)
      -> Signal<D,S>
    Iterate(const Events<D,E>& events, V&& init,
            const SignalPack<D,TDepValues...>& depPack, FIn&& func)
      -> Signal<D,S>

    // Hold the most recent event in a signal
    Hold(const Events<D,T>& events, V&& init)
      -> Signal<D,T>

    // Sets signal value to value of target when event is received
    Snapshot(const Events<D,E>& trigger, const Signal<D,S>& target)
      -> Signal<D,S>

    // Emits value changes of target signal
    Monitor(const Signal<D,S>& target)
      -> Events<D,S>

    // Emits value of target signal when event is received
    Pulse(const Events<D,E>& trigger, const Signal<D,S>& target)
      -> Events<D,S>

    // Emits token when target signal was changed
    OnChanged(const Signal<D,S>& target)
      -> Events<D,Token>

    // Emits token when target signal was changed to value
    OnChangedTo(const Signal<D,S>& target, V&& value)
      -> Events<D,Token>
}
-->