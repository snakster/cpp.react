---
layout: default
title: REACTIVE_DOMAIN
type_tag: macro
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Domain.h, url: 'reference/Domain.h/'}
---

Defines a reactive domain.

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
It has to be of type `template <EPropagationMode> class T`
If this parameter is omitted, a default engine is selected based on the given mode.

The following table lists the available engines and the modes they support:

<table>
<tr>
	<td>Engine</td>
	<td>ToposortEngine</td>
	<td>PulsecountEngine</td>
	<td>SubtreeEngine</td>
</tr>
<tr>
	<td>sequential</td>
	<td>Yes</td>
	<td>No</td>
	<td>No</td>
</tr>
<tr>
	<td>sequential_concurrent</td>
	<td>Yes</td>
	<td>No</td>
	<td>No</td>
</tr>
<tr>
	<td>parallel</td>
	<td>Yes</td>
	<td>Yes</td>
	<td>Yes</td>
</tr>
<tr>
	<td>parallel_concurrent</td>
	<td>Yes</td>
	<td>Yes</td>
	<td>Yes</td>
</tr>
</table>