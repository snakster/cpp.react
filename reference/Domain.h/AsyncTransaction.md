---
layout: default
title: DoTransaction
type_tag: function
---
## Syntax
{% highlight C++ %}
// (1)
template
<
    typename D,
    typename F
>
void AsyncTransaction(F&& func);

// (2)
template
<
    typename D,
    typename F
>
void AsyncTransaction(TurnFlagsT flags, F&& func);

// (3)
template
<
    typename D,
    typename F
>
void AsyncTransaction(TransactionStatus& status, F&& func);

// (4)
template
<
    typename D,
    typename F
>
void AsyncTransaction(TurnFlagsT flags, TransactionStatus& status, F&& func);
{% endhighlight %}

## Semantics
TODO