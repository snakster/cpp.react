---
layout: default
title: Subtree engine
groups: 
 - {name: Home, url: ''}
 - {name: Guides , url: 'guides/'}
 - {name: Propagation engines , url: 'guides/engines/'}
---

* [Motivation](#motivation)
* [Concept](#concept)
* [Issues](#issues)
* [Conclusions](#conclusions)

## Motivation

If a graph mostly consists of nodes representing lightweight operations, parallelization with Pulsecount is not economic w.r.t. used CPU time.
Sequential toposort, on the other hand, has excellent metrics in this regard, but if there are heavyweight nodes, we waste potential by not parallelizing.
The goal of the Subtree engine is to provide a balance between these two approaches by parallelizing only when encountering heavyweight nodes.


## Concept

The Subtree engine combines sequential Toposort with the initial graph marking phase of Pulsecount.

During the toposort phase, it tries to update as many lightweight nodes as possible.
If it encounters a heavyweight node, it marks the subtree with the former as root.
Nodes within a marked subtree are ignored in phase 1.

In phase 2, the Pulsecount algorithm is used to update the heavyweight subtrees.


## Issues

TODO


## Conclusions

TODO