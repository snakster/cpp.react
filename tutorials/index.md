---
layout: default
title: Tutorials
groups: 
 - {name: Home, url: ''}
---

## Preface

It's recommended to read the tutorials below the order they're listed in.

Code examples are kept to brief to focus on the integral parts.
Repeated definitions are omitted and we assume that the used types and functions from the standard library,
i.e. `std::string` or `std::cout`, are available in the current namespace.
Complete source code of some of the examples can be found [here]().

-----

# Basics

Tutorials of this category cover the core abstractions and aspects of their use.

### [Signals]({{ site.baseurl }}/tutorials/BasicSignals.html)

> Introduces signals, one of two core reactive abstractions, ...

### [Event streams]({{ site.baseurl }}/tutorials/BasicEvents.html)

> ... and event streams, which are the second one.

### [Observers]({{ site.baseurl }}/tutorials/BasicObservers.html)

> Shows how callback functions are registered to other reactive values and how their lifetime is managed.

### [Algorithms]({{ site.baseurl }}/tutorials/BasicAlgorithms.html)

> Approaches more complex problems by combining signals and events with the generic functions.

### [Class composition]({{ site.baseurl }}/tutorials/BasicComposition.html)

> Demonstrates how reactive values integrate with object-oriented design.

-----

# Parallelism and concurrency

This category contains tutorials that focus on aspects of concurrency and parallelism.

### [Enabling parallelization]({{ site.baseurl }}/tutorials/Parallelization.html)

> Introduces the different concurrency policies, how to select between them ...

### [Asynchrounous transactions and continuations]({{ site.baseurl }}/tutorials/Async.html)

> ... and how to make use of them with asynchrounous transactions and continuations.

### Performance tuning

> Shows ways to improve performance of parallel updating and concurrent input.