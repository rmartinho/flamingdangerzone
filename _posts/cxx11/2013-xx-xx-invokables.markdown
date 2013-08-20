---
layout: post
title: Invokables
categories: cxx11
short: where INVOKE materialises
published: false
---

In several places related to callable objects, the C++ standard makes use of a
magical `INVOKE(f, t1, t2, ..., tN)` expression that unifies all kinds of
callable objects (including member function pointers called with `this`
arguments as either pointers or references and member data pointers).

However, this `INVOKE` thing is not part of the language nor the standard
library. It's only a tool for defining the semantics of various constructs in
the standard text, but it isn't a tool given to programmers.

I find it to be quite useful, however.

