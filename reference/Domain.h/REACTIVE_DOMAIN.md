---
layout: default
title: REACTIVE_DOMAIN
type_tag: macro
---
## Syntax
{% highlight C++ %}
// (1)
#define REACTIVE_DOMAIN(name, mode)

// (2)
#define REACTIVE_DOMAIN(name, mode, engine)
{% endhighlight %}

## Semantics
Defines a reactive domain `name`, declared as `class name : public DomainBase<name>` and initializes static data for this domain.

The `mode` parameter has to be of type `EDomainMode`, which is defined as follows:
{% highlight C++ %}
enum EDomainMode
{
    sequential,
    sequential_concurrent,
    parallel,
	parallel_concurrent
};
{% endhighlight %}

The `engine` parameter selects an explicit propagation engine.
It has to be of type `template <EPropagationMode> class`
If this parameter is omitted, a default engine is selected based on the given mode.

The following table lists the available engines and the modes they support:

ToposortEngine
PulsecountEngine
SubtreeEngine