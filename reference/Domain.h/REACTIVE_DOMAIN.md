---
layout: default
---
# REACTIVE_DOMAIN

## Syntax
{% highlight C++ %}
#define REACTIVE_DOMAIN(name, ...)
{% endhighlight %}

## Semantics
Defines a reactive domain `name`, declared as `class name : public DomainBase<name>`. Also initializes static data for this domain.

The optional parameter is the propagation engine. If omitted, `Toposort<sequential>` is used as default.