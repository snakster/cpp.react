---
layout: default
title: 'Signals vs. callbacks'
groups: 
 - {name: Home, url: ''}
 - {name: Examples, url: ''}
---

* [Problem statement](#problem-statement)
* [Solution 1: Simple member function](#solution-1-simple-member-function)
* [Solution 2: Manually triggered re-calculation](#solution-2-manually-triggered-re-calculation)
* [Solution 3: Callbacks](#solution-3-callbacks)
* [Final solution: Signals](#final-solution-signals)

This basic example explains the motivation behind signals by comparing them to some alternative approaches.

## Problem statement
Here's a class `Shape` with two dimensions `Width` and `Height`:

{% highlight C++ %}
class Shape
{
public:
    int Width  = 0;
    int Height = 0;
};
{% endhighlight %}

The size of the shape should be calculated accordingly:
{% highlight C++ %}
int calculateSize(int width, int height) { return width * height; }
{% endhighlight %}

We want to add a method to calculate size for our shape class.


## Solution 1: Simple member function

{% highlight C++ %}
class Shape
{
public:
    int Width  = 0;
    int Height = 0;
    int Size() const { return Width * Height; }
};
{% endhighlight %}

This gets the job done, but whenever `Size()` is called, the calculation is repeated, even if the the shape's dimensions did not change after the previous call.
For this simple example that's fine, but let's assume calculating size would be an expensive operation.
We rather want to re-calculate it once after width or height have been changed and just return that result in `Size()`.


## Solution 2: Manually triggered re-calculation

{% highlight C++ %}
class Shape
{
public:
    int Width() const   { return width_; }
    int Height() const  { return height_; }
    int Size() const    { return size_; }

    void SetWidth(int v)
    {
        if (width_ == v) return;
        width_ = v;
        updateSize(); 
    }
    
    void SetHeight(int v)
    {
        if (height_ == v) return;
        height_ = v;
        updateSize();
    }

private:
    void updateSize()   { size_ = width_ * height_; }

    int width_  = 0;
    int height_ = 0;
    int size_   = 0;
};
{% endhighlight %}
{% highlight C++ %}
Shape myShape;

// Set dimensions
myShape.SetWidth(20);
myShape.SetHeight(20);

// Get size
auto curSize = myShape.Size();
{% endhighlight %}

We've declared the data fields as private members and exposed them through getter and setter functions,
so we can call `updateSize()` internally, after width or height have been changed.

When considering where we started from, this adds quite a bit of boilerplate code, and as usual when having to do things manually, we can make mistakes.

What if more dependent attributes should be added?
Using the current approach, updates are manually triggered from the dependencies.

This requires changing all dependencies when adding a new dependent values, which gets increasingly complex.
More importantly, it's not an option, if the dependent values are not known yet or could be added and removed dynamically.
A common approach to enable this are callbacks.


## Solution 3: Callbacks

{% highlight C++ %}
class Shape
{
public:
    using CallbackT = std::function<void(int)>;

    int Width() const   { return width_; }
    int Height() const  { return height_; }
    int Size() const    { return size_; }

    void SetWidth(int v)
    {
        if (width_ == v) return;
        width_ = v;
        updateSize(); 
    }
    
    void SetHeight(int v)
    {
        if (height_ == v) return;
        height_ = v;
        updateSize();
    }

    void AddSizeChangeCallback(const CallbackT& f)   { sizeCallbacks_.push_back(f); }

private:
    void updateSize()
    {
        auto oldSize = size_;
        size_ = width_ * height_;
        
        if (oldSize != size_)
            notifySizeCallbacks();
    }

    void notifySizeCallbacks()
    {
        for (const auto& f : sizeCallbacks_)
            f(size_);
    }

    int width_  = 0;
    int height_ = 0;
    int size_   = 0;

    std::vector<CallbackT> sizeCallbacks_;
};
{% endhighlight %}
{% highlight C++ %}
Shape myShape;

// Callback on change
myShape.AddSizeChangeCallback([] (int newSize) {
    redraw();
});
{% endhighlight %}

For brevity, this example includes callbacks for size changes, but not for width and height.
Nonetheless, it adds even more boilerplate.
Instead of implementing the callback mechanism ourselves, we can use external libraries for that, for example `boost::signals2`, which handles storage and batch invocation of callbacks;
but overall, it has no impact on the design.

To summarize some of the pressing issues with the solutions shown so far:

* Error-proneness: There is no guarantee that `size == width * height`. It's only true as long as we don't forget to call `updateSize()` after changes.
* Boilerplate: Check against previous value, trigger update of dependent internal values, trigger callback notification, register callbacks, ...
* Complexity: Adding new dependent attributes requires changes in existing functions and potentially adding additional callback holders.

What it boils down to, is that the change propagation must be handled by hand.
The next example shows how signals can be used for this scenario.


## Final solution: Signals

{% highlight C++ %}
#include "react/Domain.h"
#include "react/Signal.h"
#include "react/Observer.h"

using namespace react;

REACTIVE_DOMAIN(D)

class Shape
{
public:
    USING_REACTIVE_DOMAIN(D)

    VarSignalT<int> Width   = MakeVar<D>(0);
    VarSignalT<int> Height  = MakeVar<D>(0);
    SignalT<int>    Size    = Width * Height;
};
{% endhighlight %}

`Size` now behaves like a function of `Width` and `Height`, similar to Solution 1.
But behind the scenes, it works like Solution 2, i.e. size is only only re-calculated when width or height change.

The following code shows how to interact with these signals:
{% highlight C++ %}
Shape myShape;

// Set dimensions
myShape.Width.Set(20);
myShape.Height.Set(20);

// Get size
auto curSize = myShape.Size(); // Or more verbose: myShape.Size.Value()
{% endhighlight %}

Every reactive value automatically supports registration of callbacks (they are called observers):
{% highlight C++ %}
// Callback on change
Observe(myShape.Size, [] (int newSize) {
    redraw();
});

// Those would work, too
Observe(myShape.Width, [] (int newWidth) { ... });
Observe(myShape.Height, [] (int newHeight) { ... });
{% endhighlight %}