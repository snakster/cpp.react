---
layout: default
title: Reactive types
groups: 
 - {name: Home, url: ''}
 - {name: Guides , url: 'guides/'}
---

The [graph model guide]() explained how reactive values form a graph, with data being propagated along its edges.
It did not concretize what "value" or "data" actually means, because the dataflow perspective allows to abstract from such details.

In this guide, we lower the level of abstraction and introduce the core reactive types that make up this library.
Namely those are:

- Signals
- Event streams
- Observers
- Continuations
- Reactors

## Signals

Signals are time-varying reactive values.
To explain this by example, consider the following imperative code:
{% highlight C++ %}
int a = 1;
int b = 2;
int x = a + b;

print(x); // output: 3
a = 2;
print(x); // output: 3
{% endhighlight %}

Unsurprisingly, after `a` was changed, `x` is still 3.
`=` and `+` are a prettier notation for what looks like this at instruction level:
{% highlight C++ %}
mov a, 1	// a = 1
mov b, 2	// b = 1
mov x, a 
add x, b	// x = a + b
push x
call print	// print(x)
mov a, 2	// a = 2
push x
call print	// print(x)
{% endhighlight %}

In mathematics, however, `=` and `+` would declare `x` as a function `f: x = a + b`.
{% highlight C++ %}
VarSignal<int> a = 1;
VarSignal<int> b = 2;
Signal<int>    x = a + b;

print(x); // output: 3
a = 2;
print(x); // output: 4
{% endhighlight %}



## Event streams



## Observers



## Continuations



## Reactors