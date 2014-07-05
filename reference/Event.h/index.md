---
layout: default
title: Event.h
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
---
Contains the event stream template classes and functions.

## Header
{% highlight C++ %}
`#include "react/Event.h"`
{% endhighlight %}

## Synopsis

### Classes

<div class="highlight"><pre><code class="c++"><span class="k">namespace</span> <span class="n">react</span>
<span class="p">{</span>
    <span class="c1">// Token</span>
    <span class="k">enum</span> <span class="k">class</span> <a class="code_link" href="{{ site.baseurl }}/reference/Event.h/Token.html">Token</a>

    <span class="c1">// Events</span>
    <span class="k">class</span> <a class="code_link" href="{{ site.baseurl }}/reference/Event.h/Events.html">Events</a><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">E</span><span class="o">=</span><span class="n">Token</span><span class="o">&gt;</span>

    <span class="c1">// EventSource</span>
    <span class="k">class</span> <a class="code_link" href="{{ site.baseurl }}/reference/Event.h/EventSource.html">EventSource</a><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">E</span><span class="o">=</span><span class="n">Token</span><span class="o">&gt;</span>

    <span class="c1">// TempEvents</span>
    <span class="k">class</span> <a class="code_link" href="{{ site.baseurl }}/reference/Event.h/TempEvents.html">TempEvents</a><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">E</span><span class="p">,</span><span class="cm">/*unspecified*/</span><span class="o">&gt;</span>
<span class="p">}</span>
</code></pre></div>

<!--
{% highlight C++ %}
namespace react
{
    // Token
    enum class Token

    // Events
    class Events<D,E=Token>

    // EventSource
    class EventSource<D,E=Token>

    // TempEvents
    class TempEvents<D,E,/*unspecified*/>
}
{% endhighlight %}
-->

### Functions

<div class="highlight"><pre><code class="c++"><span class="k">namespace</span> <span class="n">react</span>
<span class="p">{</span>
    <span class="c1">// Creates a new event source</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Event.h/MakeEventSource.html">MakeEventSource</a><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">E</span><span class="o">=</span><span class="n">Token</span><span class="o">&gt;</span><span class="p">()</span>
      <span class="o">-&gt;</span> <span class="n">EventSource</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">E</span><span class="o">&gt;</span>

    <span class="c1">// Creates a new event stream that merges events from given streams</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Event.h/Merge.html">Merge</a><span class="p">(</span><span class="k">const</span> <span class="n">Events</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">E</span><span class="o">&gt;&amp;</span> <span class="n">arg1</span><span class="p">,</span> <span class="k">const</span> <span class="n">Events</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">TValues</span><span class="o">&gt;&amp;</span> <span class="p">...</span> <span class="n">args</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">TempEvents</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">E</span><span class="p">,</span><span class="cm">/*unspecified*/</span><span class="o">&gt;</span>

    <span class="c1">// Creates a new event stream that filters events from other stream</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Event.h/Filter.html">Filter</a><span class="p">(</span><span class="k">const</span> <span class="n">Events</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">E</span><span class="o">&gt;&amp;</span> <span class="n">source</span><span class="p">,</span> <span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">TempEvents</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">E</span><span class="p">,</span><span class="cm">/*unspecified*/</span><span class="o">&gt;</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Event.h/Filter.html">Filter</a><span class="p">(</span><span class="n">TempEvents</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">E</span><span class="p">,</span><span class="cm">/*unspecified*/</span><span class="o">&gt;&amp;&amp;</span> <span class="n">source</span><span class="p">,</span> <span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">TempEvents</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">E</span><span class="p">,</span><span class="cm">/*unspecified*/</span><span class="o">&gt;</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Event.h/Filter.html">Filter</a><span class="p">(</span><span class="k">const</span> <span class="n">Events</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">E</span><span class="o">&gt;&amp;</span> <span class="n">source</span><span class="p">,</span> <span class="k">const</span> <span class="n">SignalPack</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">TDepValues</span><span class="p">...</span><span class="o">&gt;&amp;</span> <span class="n">depPack</span><span class="p">,</span>
           <span class="n">FIn</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">Events</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">E</span><span class="o">&gt;</span>

    <span class="c1">// Creates a new event stream that transforms events from other stream</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Event.h/Transform.html">Transform</a><span class="p">(</span><span class="k">const</span> <span class="n">Events</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">E</span><span class="o">&gt;&amp;</span> <span class="n">source</span><span class="p">,</span> <span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">TempEvents</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">T</span><span class="p">,</span><span class="cm">/*unspecified*/</span><span class="o">&gt;</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Event.h/Transform.html">Transform</a><span class="p">(</span><span class="n">TempEvents</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">E</span><span class="p">,</span><span class="cm">/*unspecified*/</span><span class="o">&gt;&amp;&amp;</span> <span class="n">source</span><span class="p">,</span> <span class="n">F</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">TempEvents</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">T</span><span class="p">,</span><span class="cm">/*unspecified*/</span><span class="o">&gt;</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Event.h/Transform.html">Transform</a><span class="p">(</span><span class="k">const</span> <span class="n">Events</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">TIn</span><span class="o">&gt;&amp;</span> <span class="n">source</span><span class="p">,</span> <span class="k">const</span> <span class="n">SignalPack</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">TDepValues</span><span class="p">...</span><span class="o">&gt;&amp;</span> <span class="n">depPack</span><span class="p">,</span>
              <span class="n">FIn</span><span class="o">&amp;&amp;</span> <span class="n">func</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">Events</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">T</span><span class="o">&gt;</span>

    <span class="c1">// Creates a new event stream by flattening a signal of an event stream</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Event.h/Flatten.html">Flatten</a><span class="p">(</span><span class="k">const</span> <span class="n">Signal</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">Events</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">T</span><span class="o">&gt;&gt;&amp;</span> <span class="n">other</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">Events</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">T</span><span class="o">&gt;</span>

    <span class="c1">// Creates a token stream by transforming values from source to tokens</span>
    <a class="code_link" href="{{ site.baseurl }}/reference/Event.h/Tokenize.html">Tokenize</a><span class="p">(</span><span class="n">TEvents</span><span class="o">&amp;&amp;</span> <span class="n">source</span><span class="p">)</span>
      <span class="o">-&gt;</span> <span class="n">TempEvents</span><span class="o">&lt;</span><span class="n">D</span><span class="p">,</span><span class="n">Token</span><span class="p">,</span><span class="cm">/*unspecified*/</span><span class="o">&gt;</span>
<span class="p">}</span>
</code></pre></div>

<!--
{% highlight C++ %}
namespace react
{
    // Creates a new event source
    MakeEventSource<D,E=Token>()
      -> EventSource<D,E>

    // Creates a new event stream that merges events from given streams
    Merge(const Events<D,E>& arg1, const Events<D,TValues>& ... args)
      -> TempEvents<D,E,/*unspecified*/>

    // Creates a new event stream that filters events from other stream
    Filter(const Events<D,E>& source, F&& func)
      -> TempEvents<D,E,/*unspecified*/>
    Filter(TempEvents<D,E,/*unspecified*/>&& source, F&& func)
      -> TempEvents<D,E,/*unspecified*/>
    Filter(const Events<D,E>& source, const SignalPack<D,TDepValues...>& depPack,
           FIn&& func)
      -> Events<D,E>

    // Creates a new event stream that transforms events from other stream
    Transform(const Events<D,E>& source, F&& func)
      -> TempEvents<D,T,/*unspecified*/>
    Transform(TempEvents<D,E,/*unspecified*/>&& source, F&& func)
      -> TempEvents<D,T,/*unspecified*/>
    Transform(const Events<D,TIn>& source, const SignalPack<D,TDepValues...>& depPack,
              FIn&& func)
      -> Events<D,T>

    // Creates a new event stream by flattening a signal of an event stream
    Flatten(const Signal<D,Events<D,T>>& other)
      -> Events<D,T>

    // Creates a token stream by transforming values from source to tokens
    Tokenize(TEvents&& source)
      -> TempEvents<D,Token,/*unspecified*/>
}
{% endhighlight %}
-->