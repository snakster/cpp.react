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

## Motivation

The Subtree engine is a hybrid approach of Toposort and Pulsecount.


## Concept

The Subtree engine combines sequential Toposort with the initial graph marking phase of Pulsecount.

During the toposort phase, it tries to update as many lightweight nodes as possible.
If it encounters a heavyweight node, it marks the subtree with the former as root.
Nodes within a marked subtree are ignored in phase 1.

In phase 2, the Pulsecount algorithm is used to update the heavyweight subtrees.