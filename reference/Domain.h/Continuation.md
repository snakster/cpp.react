---
layout: default
title: Continuation
type_tag: class
groups: 
 - {name: Home, url: ''}
 - {name: Reference , url: 'reference/'}
 - {name: Domain.h, url: 'reference/Domain.h/'}
---
This class manages ownership of a continuation. A continuation is a link between two domains (including from a domain to itself).

Continuations are created with [MakeContinuation](MakeContinuation.html).

## Synopsis
{% highlight C++ %}
template
<
    typename D,
    typename D2
>
class Continuation
{
    // Type aliases
    using SourceDomainT = D;
    using TargetDomainT = D2;

    // Constructor
    Continuation();
    Continuation(Continuation&& other); // Move

    // Assignment
    Continuation& operator=(Continuation&& other); // Move

    // Tests if this instance is linked to a node
    bool IsValid() const;

    // Sets weight override for linked node
    void SetWeightHint(WeightHint hint);
};
{% endhighlight %}

## Template parameters
<table class="wide_table">
<tr>
<td class="descriptor_cell">D</td>
<td>The domain this continuation belongs to. Aliased as member type <code>SourceDomainT</code>.</td>
</tr>
<tr>
<td class="descriptor_cell">D2</td>
<td>Signal value type. Aliased as member type <code>TargetDomainT</code>.</td>
</tr>
</table>