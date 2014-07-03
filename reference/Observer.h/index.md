---
layout: default
title: Observer.h
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
---
Contains the observer template classes and functions.

## Header
`#include "react/Observer.h"`

## Synopsis

### Classes

<div class="highlight"><pre><code class="c++"><span class="k">namespace</span> <span class="n">react</span>
<span class="p">{</span>
    <span class="c1">// ObserverAction</span>
    <span class="k">enum</span> <span class="k">class</span> <a class="code_link" href="{{ site.baseurl }}/reference/Observer.h/ObserverAction.html">ObserverAction</a>

    <span class="c1">// Observer</span>
    <span class="k">class</span> <a class="code_link" href="{{ site.baseurl }}/reference/Observer.h/Observer.html">Observer</a><span class="o">&lt;</span><span class="n">D</span><span class="o">&gt;</span>

    <span class="c1">// ScopedObserver</span>
    <span class="k">class</span> <a class="code_link" href="{{ site.baseurl }}/reference/Observer.h/ScopedObserver.html">ScopedObserver</a><span class="o">&lt;</span><span class="n">D</span><span class="o">&gt;</span>
<span class="p">}</span>
</code></pre></div>

<!--
{% highlight C++ %}
namespace react
{
    // ObserverAction
    class ObserverAction
    
    // Observer
    class Observer<D>;

    // ScopedObserver
    class ScopedObserver<D>;
}
{% endhighlight %}
-->

### Functions

<div class="highlight"><pre><code class="c++"><span class="k">namespace</span> <span class="n">react</span>
<span class="p">{</span>
    <span class="c1">// Creates an observer of the given subject</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Observer.h/Observe.html">Observe</a><span class="p">(</span><span class="k">const</span> <span class="n">Signal</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">S</span><span class="o">&gt;&amp;</span> <span class="n">subject</span><span class="p">,</span> <span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">Observer</span><span class="o">&lt;</span><span class="n">D</span><span class="o">&gt;</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Observer.h/Observe.html">Observe</a><span class="p">(</span><span class="k">const</span> <span class="n">Events</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">E</span><span class="o">&gt;&amp;</span> <span class="n">subject</span><span class="p">,</span> <span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">Observer</span><span class="o">&lt;</span><span class="n">D</span><span class="o">&gt;</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Observer.h/Observe.html">Observe</a><span class="p">(</span><span class="k">const</span> <span class="n">Events</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">E</span><span class="o">&gt;&amp;</span> <span class="n">subject</span><span class="p">,</span>
            <span class="k">const</span> <span class="n">SignalPack</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">TDepValues</span><span class="p">...</span><span class="o">&gt;&amp;</span> <span class="n">depPack</span><span class="p">,</span> <span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">Observer</span><span class="o">&lt;</span><span class="n">D</span><span class="o">&gt;</span>
<span class="p">}</span>
</code></pre></div>

<!--
{% highlight C++ %}

namespace react
{
    // Creates an observer of the given subject
    Observe(const Signal<D,S>& subject, F&& func)
      -> Observer<D>
    Observe</a>(const Events<D,E>& subject, F&& func)
      -> Observer<D>
    Observe</a>(const Events<D,E>& subject,
            const SignalPack<D,TDepValues...>& depPack, F&& func)
      -> Observer<D>
}
{% endhighlight %}
-->