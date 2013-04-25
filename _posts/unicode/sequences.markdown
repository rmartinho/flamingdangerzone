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

blah blah about output iterators being crappy and non-composable blah blah
blah blah about implementing iterators being painful but Boost.Iterator
mitigating it blah blah

### Boost.Range

I sought out ranges, and looked into Boost. Soon I changed my existing
interfaces to be based on [Boost.Range].

Soon enough I learned that Boost.Range is actually rather poor. It provides a
nice-looking interface for various operations, but it falls very short as soon
as one needs something non-trivial.

blah blah about one-to-many/many-to-one filters and about Boost.Range being
actually iterators all over again blah blah

### Taussig

Eventually it reached a point where I felt like I was fighting the abstraction
too often, so I went back to the drawing board.

blah blah Taussig is going to be awesome blah blah

 [ogonek]: http://github.com/rmartinho/ogonek
 [Boost.Range]: http://www.boost.org/libs/range/doc/html/index.html

