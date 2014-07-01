---
layout: default
title: Continuation
type_tag: class
---
This class manages ownership of a continuation. A continuation is a link between two domains (including from a domain to itself).

## Synopsis
{% highlight C++ %}
namespace react
{
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
        Continuation(Continuation&& other);

        // Assignemnt
        Continuation& operator=(Continuation&& other);
    };
}
{% endhighlight %}

## Template parameters
<table>
<tr>
<td>D</td>
<td>The domain this continuation belongs to. Aliased as member type <code>SourceDomainT</code>.</td>
</tr>
<tr>
<td>D2</td>
<td>Signal value type. Aliased as member type <code>TargetDomainT</code>.</td>
</tr>
</table>