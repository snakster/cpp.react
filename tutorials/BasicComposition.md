---
layout: default
title: Class composition
groups: 
 - {name: Home, url: ''}
 - {name: Tutorials , url: 'tutorials/'}
---

- [Reactive class members](#Reactive-class-members)
- [Signals/events of references](#signalsevents-of-references)
- [Dynamic signal/event references](#dynamic-signalevent-references)

## Reactive class members

Most examples in previous tutorials defined reactives in a global context.
In practice, however, the predominant use case should be defining them as class members (hopefully).
There is nothing fundamentally different about that.

Here's an example:
{% highlight C++ %}
#include "react/Domain.h"
#include "react/Signal.h"
#include "react/Event.h"

REACTIVE_DOMAIN(D, sequential)

class Shape
{
public:
    USING_REACTIVE_DOMAIN(D)

    VarSignalT<int>  Width    = MakeVar<D>(0);
    VarSignalT<int>  Height   = MakeVar<D>(0);

    SignalT<int>     Size     = width * height;

    EventSourceT<>   HasMoved = MakeEventSource<D>();
};
{% endhighlight %}

We used C++11 in-class member initialization.
Alternatively, the members could've been initialized the constructor.
For both variants, it's important to mind the declaration/initialization order:
{% highlight C++ %}
class Shape
{
public:
    USING_REACTIVE_DOMAIN(D)

    SignalT<int>    Size;
    VarSignalT<int> Width;
    VarSignalT<int> Height;
    EventSourceT<>  HasMoved;

    Shape() :
        // Breaks, because Width and Height are still uninitialized at this point
        Size( Width * Height ), 
        Width( MakeVar<D>(0) ),
        Height( MakeVar<D>(0) ),
        HasMoved( MakeEventSource<D>() )
    {}
};
{% endhighlight %}

If a reactive value is used before it has been initialized, that would be equivalent to a `nullptr`.


## Signals/events of references

When creating signals (or event streams) of class types, chances are we want references or pointers as value types.

Signals of pointers require no special treatment, since pointers have value semantics anyway.
For references, we could use `std::reference_wrapper`. Alternatively, `Signal` is specialized for reference types, which is slightly more convenient:
{% highlight C++ %}
class Company
{
public:
    const char* Name;

    Company(const char* name) :
        Name( name )
    {}
};

class Employee
{
public:
    USING_REACTIVE_DOMAIN(D)

    VarSignalT<Company&> MyCompany;

    Employee(Company& company) :
        MyCompany( MakeVar<D>( std::ref(company) ) )
    {}
};
{% endhighlight %}
{% highlight C++ %}
Company     company1( "MetroTec" );
Company     company2( "ACME" );

Employee    bob( company1 );

Observe(bob.MyCompany, [] (const Company& company) {
    cout << "Bob works for " << company.Name << endl;
});

bob.Company <<= std::ref(company2); // output: Bob now works for ACME
{% endhighlight %}

As shown, input to a signal of a reference has to be wrapped by `std::ref` or `std::cref` to make the reference obvious.
Event streams of references are used in the same fashion.


## Dynamic signal/event references

Now let's consider that the company name is a signal as well:
{% highlight C++ %}
#include <string>
using std::string;

class Company
{
public:
    USING_REACTIVE_DOMAIN(D)

    VarSignalT<string> Name;

    Company(const char* name) :
        Name( MakeVar<D>(string( name ) )
    {}
};
{% endhighlight %}
{% highlight C++ %}
Company     company1{ "MetroTec" };
Company     company2{ "ACME" };

Employee    alice{ company1 };
{% endhighlight %}
We want to create an observer of the name of Alice's current company. This might work:
{% highlight C++ %}
Observe(
    alice.MyCompany.Value().Name,
    [] (string name) {
        cout << "Alice works for " << name << endl;
    });
{% endhighlight %}
But the following input reveals a problem:
{% highlight C++ %}
company1.Name <<= string("ModernTec");
// output: Alice now works for ModernTec
// OK so far

alice.Company <<= std::ref(company2); 
// no output
// Name should've changed
{% endhighlight %}
The observer was registered to the name of the company Alice worked for at the time, as indicated by `MyCompany.Value()`.
When the company changes from `company1` to `company2`, it has to shift from `company1.Name` to `company2.Name`.
We cannot do this with any of the mechanisms presented so far.

This is enabled by the `REACTIVE_REF` macro:
{% highlight C++ %}
SignalT<string> myCompanyName = REACTIVE_REF(alice.MyCompany, Name);

Observe(myCompanyName, [] (const string& name) {
    cout << "Alice works for " << name << endl;
});
{% endhighlight %}
The intermediate signal can be avoided, as long as we keep the observer handle alive:
{% highlight C++ %}
ObserverT obs = Observe(
    REACTIVE_REF(alice.MyCompany, Name),
    [] (const string& name) {
        cout << "Alice works for " << name << endl;
    });
{% endhighlight %}

In both cases, the output is:
{% highlight C++ %}
company1.Name <<= string("ModernTec");
// output: Alice now works for ModernTec

alice.Company <<= std::ref(company2); 
// output: Alice now works for ACME

company2.Name <<= string("A.C.M.E."); 
// output: Alice now works for A.C.M.E.
{% endhighlight %}

A similar macro `REACTIVE_PTR` exists for pointer types (i.e. `VarSignalT<Company*>`).