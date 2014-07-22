---
layout: default
title: Signals
groups: 
 - {name: Home, url: ''}
 - {name: Tutorials , url: 'tutorials/'}
---

- [Defining a domain](#defining-a-domain)
- [Hello world](#hello-world)
- [Reacting to value changes](#reacting-to-value-changes)
- [Changing multiple inputs](#changing-multiple-inputs)
- [Modifying inputs in-place](#modifying-inputs-in-place)

## Defining a domain

Each reactive value belongs to a logical domain. The purpose of a domain is

* grouping related reactive values together and encapsulating them;
* allowing different concurrency policies for separate domains;
* simplifying management of dependency relations by splitting up the dependency graph into smaller parts.

Hence, the first thing we do is defining a domain:

{% highlight C++ %}
#include "react/Domain.h"

using namespace react;

REACTIVE_DOMAIN(MyDomain, sequential)
{% endhighlight %}

The first parameter of the `REACTIVE_DOMAIN` macro is the domain name.
Technically, a domain is a type, so it can be aliased or used as a type parameter for templates.
Think of it as being declared as `class MyDomain : /*...*/`.

The second parameter specifies the concurrency policy, which controls

* concurrent input, and
* parallel updating.

For this tutorial, neither is relevant, so we use `sequential`.


## Hello world

Next, we create a simple signal that holds the string `"Hello world"`:
{% highlight C++ %}
#include "react/Domain.h"
#include "react/Signal.h"

using namespace react;

REACTIVE_DOMAIN(D, sequential)

VarSignal<D,string> myString = MakeVar<D>(string( "Hello world" ));
{% endhighlight %}

`D` is used as the domain name for its shortness, as it will be for the remainder of this tutorial.

Conceptionally, an instance of type `Signal<D,S>` is a reactive container that holds a single value of type `S` and belongs to domain `D`.
`myString` is declared as a `VarSignal`, which allows us to modify its value later; declaring it as `Signal` would've made it read-only.

The `MakeVar<D>` function takes a value of type `S` and returns a new `VarSignal<D,S>`, which initially holds the given value.

`myString` is similar to an ordinary variable; its value can be read imperatively with `Value()` and changed with `Set(x)`.
An alternative to `myString.Set(x)` is the overloaded `<<=` operator, i.e. `myString <<= x`.
This should not be mixed up with the assignment operator, which is reserved to assign the signal itself, not the inner value.

So far that's not very reactive - let's make it more interesting.
First, we define a helper function to concatenate two strings and add whitespace in between:
{% highlight C++ %}
string concatFunc(string first, string second) {
    return first + string(" ") + second;
};
{% endhighlight %}

With this, we replace the `myString` definition from the previous example with the following:
{% highlight C++ %}
USING_REACTIVE_DOMAIN(D)

// The two words
VarSignalT<string> firstWord  = MakeVar<D>(string( "Change" ));
VarSignalT<string> secondWord = MakeVar<D>(string( "me!" ));
 
// A signal that concatenates both words
SignalT<string> bothWords =
    MakeSignal(
        With(firstWord,secondWord),
        concatFunc);
{% endhighlight %}

The macro `USING_REACTIVE_DOMAIN(name)` defines aliases for reactive types of the given domain in the current scope.
This allows us to use `VarSignalT<S>` instead of `VarSignal<D,S>`.

`MakeSignal` connects the values of the signals in the `With(...)` expression to the function arguments of `concatFunc`.
The value type of the created signal matches the return type of the function.
It's set by implicitly calling `concatFunc(firstWord.Value(), secondWord.Value())`. This happens

- to set the initial value upon initialization of `bothWords`, and
- when `firstWord` or `secondWord` have been changed.

Here's a demonstration:
{% highlight C++ %}
cout << bothWords.Value() << endl; // output: "Change me!"

firstWord  <<= string( "Hello" );
secondWord <<= string( "world" );

cout << bothWords.Value() << endl; // output: "Hello world"
{% endhighlight %}

`MakeSignal` accepts any type of function, including lambdas or `std::bind` expressions:
{% highlight C++ %}
SignalT<string> bothWords =
    MakeSignal(
        With(firstWord, secondWord),
        [] (string first, string second) {
            return first + string(" ") + second;
        });
{% endhighlight %}

The `With` utility function creates a `SignalPack` from a variable number of given signals.
A single signal can be used directly as the first argument, but multiple signals are wrapped by `With` instead of passing them directly.

The example can be made more concise, because the body of `concatFunc` consists of operators only:
{% highlight C++ %}
SignalT<string> bothWords = firstWord + string( " " ) + secondWord;
{% endhighlight %}
The definition of `bothWords` now essentially is the body of the function used to calculate it.
Most unary and binary operators are overloaded to automatically lift expressions with a signal operand to create new signals.

Would this work, too?
{% highlight C++ %}
bothWords <<= "Hello world?";
{% endhighlight %}

No, it wouldn't, because `bothWords` is of type `Signal` and not `VarSignal`.
Either the value of a signal represents a function result, or its value is set imperatively, but it can't be both.


## Reacting to value changes

In the previous example, new values were pushed with `<<=`, then the result was pulled with `Value()`.
There are some issues with this approach:

* Concurrent pushes and pulls are not thread-safe, so they have to be coordinated somehow;
* `Value()` provides no means to react to value changes (unless you count in polling).

There can be situations where the use of `Value()` is appropriate and we used it in the initial examples to demonstrate the basic idea behind signals.
However, in most cases a push-based approach should be preferred.

To demonstrate how this works, let's modify the previous example so it prints out the value of `bothWords` whenever it changes.

Technically, we could add the output during the computation itself:
{% highlight C++ %}
// Note: Don't actually do this.
SignalT<string> bothWords =
    MakeSignal(
        With(firstWord, secondWord),
        [] (string first, string second) {
            auto result = first + string(" ") + second;
            cout << result << endl;
            return result;
        });
{% endhighlight %}

This is problematic, because now the function to compute `bothWords` is no longer a pure function.
In general, a function used to calculate a signal value should do only that; it should not cause side effects or depend on values other than its arguments.
This makes it easier to reason about the program behavior, and more importantly, calculations can be parallelized without having to worry about data races.

The proper way of applying side effects is to create an `Observer`, which is essentially a callback function that is invoked whenever its subject changes:
{% highlight C++ %}
#include "Observer.h"

Observe(bothWords, [] (string s) {
    cout << s << endl;
});
{% endhighlight %}

The new value is passed as an argument to the callback function.
Observers are meant to cause side effects. They don't return any value, so it's in fact all they can do.

Testing this example is going to yield the following results:

{% highlight C++ %}
firstWord  <<= string( "Hello" );
// output: "Hello me!"

secondWord <<= string( "world" );
// output: "Hello world"

secondWord <<= string( "universe" );
// output: "Hello universe"

firstWord <<= string( "Hello" );
// no output, bothWords is still "Hello universe" and the callback is only invoked on changes
{% endhighlight %}

By default, the lifetime of an observer is attached to its observed subject (in this case, that's `bothWords`).
It's also possible to detach observers manually, but this topic is covered in another tutorial.


## Changing multiple inputs

In the previous example, `firstWord` and `secondWord` were changed separately, which resulted in an output for each change.

To explore this behaviour further, let's have a look at another example:

{% highlight C++ %}
VarSignalT<int> a = MakeVar<D>(1);
VarSignalT<int> b = MakeVar<D>(1);

SignalT<int>     x = a + b;
SignalT<int>     y = a + b;
SignalT<int>     z = x + y;
{% endhighlight %}
{% highlight C++ %}
Observe(z, [] (int newValue) {
    cout << "z changed to " << newValue << endl;
});
{% endhighlight %}
{% highlight C++ %}
a <<= 2;
{% endhighlight %}

How many outputs does this generate?

When `a` is changed, the new value is propagated to `x` and `y`.
`z` depends on both of them, so it could be updated twice.
But there is just going to be a single observable output: z changed to 6.

Here's why:
After each input (e.g. `a <<= 2`), the resulting changes are propagated to all dependent signals until all values are consistent again.
This process is called a `turn`.
Propagation is guaranteed to be _update-minimal_, which means that during a **single** turn, no value is going to be re-calculated more than once.
This includes that no observer is going be called more than once.

Returning to the previous example, we can see that changing both inputs separately results in two turns:
{% highlight C++ %}
firstWord  <<= string( "Hello" ); // Turn #1
secondWord <<= string( "world" ); // Turn #2
{% endhighlight %}

To combine those in a single turn, we can wrap them in a transaction: 
{% highlight C++ %}
DoTransaction<D>([] {
    firstWord  <<= string( "Hello" );
    secondWord <<= string( "world" );
});
// Turn #1, output: Hello world
{% endhighlight %}

Input inside the function passed to `DoTransaction` does not immediately start a turn, but rather waits until the function returns.
Then, the changes of all inputs are propagated in a single turn.
Besides avoiding redundant calculations, this allows to process related inputs together.

If there are multiple inputs to the same signal in a single transaction, only the last one is applied:
{% highlight C++ %}
VarSignalT<int> a = MakeVar<D>(1);

DoTransaction<D>([] {
    a <<= 2;
    a <<= 1; 
});
// still 1, no change and no turn
{% endhighlight %}


## Modifying inputs in-place

`VarSignals` require imperative input.
So far, we've used `Set` (or the `<<=` equivalent notation) to do that, but there might be situations where we want to modify the current signal value rather than replacing it:
{% highlight C++ %}
VarSignalT<vector<string>> data = 
    MakeVar<D>(vector<string>{ });
{% endhighlight %}
{% highlight C++ %}
auto v = data.Value(); // Copy
v.push_back("Hello");  // Change
data <<= std::move(v); // Replace (using move to avoid the extra copy)
{% endhighlight %}
Using this method, the new and old values will be compared internally, so in summary thats one copy, one comparison and two moves (one for the input, one after the comparison to apply the new value).

The following method allows us to eliminate these intermediate steps by modifying the current value in-place:
{% highlight C++ %}
data.Modify([] (vector<string>& data) {
    data.push_back("Hello");
});

data.Modify([] (vector<string>& data) {
    data.push_back("world");
});

for (const auto& s : data.Value())
    cout << s << endl;
// output: Hello world
{% endhighlight %}
The drawback is that since we can not compare the old and new values, we loose the ability to detect whether the data was actually changed.
We always have to assume that it did and re-calculate dependent signals.