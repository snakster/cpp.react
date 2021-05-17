# ![C++React](http://snakster.github.io/cpp.react//media/logo_banner3.png)

C++React is reactive programming library for C++14. It enables the declarative definition of data dependencies between state and event flows.
Based on these definitions, propagation of changes is handled automatically.

Here's a simple example:

```
using namespace react;

void AddNumbers(int a, int b)  { return a + b; }

// Two state variable objects. You can change their values manually.
auto a = StateVar<int>::Create(0);
auto b = StateVar<int>::Create(0);

// Another state object. Its value is calculated automatically based on the given function and arguments.
// If the arguments change, the value is re-calculated.
auto sum = State<int>::Create(AddNumbers, a, b);

// sum == 0
a.Set(21);
// sum == 21
b.Set(21);
// sum == 42
```

The underlying system constructs a dependency graph to collect which values are affected by a change, and in which order they have to be re-calculated.
This guarantees several properties:
- _correctness_ - no updates are forgotten;
- _consistency_ - no value is updated before all its incoming dependencies have been updated;
- _efficiency_ - no value is re-calculated more than once per update cycle, and changes are only propagated along paths where a new value is different from the old one.

The system also knows when it's safe to update values in parallel from multiple threads, and it can do live profiling to decide when that's worthwhile.

## Development status

I'm currently in the process of rewriting the library more or less from scratch and many things are still broken or outdated.

[The old, but stable and documented version is still available in this branch.](https://github.com/snakster/cpp.react/tree/legacy1)
