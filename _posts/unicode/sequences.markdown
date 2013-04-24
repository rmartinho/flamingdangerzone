---
layout: post
title: Sequences
categories: unicode
short: where I explain why I don't like iterators
---

When I started designing [ogonek] I knew that manipulating sequences of things
would be its bread and butter. Bytes, code units, code points, grapheme
clusters, words, sentences, line breaks opportunities... you name it: sequences
are all over the place, in one form or another.

### Iterators

The obvious first thing that I considered using were iterators: iterators are
the standard C++ tool for operating on sequences. The standard library
algorithms all work with iterators, and all the standard containers provide an
iterator interface.

That is a strong reason to use iterators; however, I discarded the idea early in
the design process. The main reason is that iterators don't lend themselves
easily to composition.

blah blah about output iterators blah blah

### Boost.Range

I sought out ranges, and looked into Boost. Soon I changed my existing
interfaces to be based on [Boost.Range].

Soon enough I learned that Boost.Range is actually rather poor. It provides a
nice-looking 

### Taussig

Taussig is going to be awesome.

 [ogonek]: http://github.com/rmartinho/ogonek
 [Boost.Range]: http://www.boost.org/libs/range/doc/html/index.html

