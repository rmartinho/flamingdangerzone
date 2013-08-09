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
the design process. The primary reason for that choice was the fact that
iterators don't lend themselves easily to composition.

Most algorithms in ogonek take sequences as arguments and produce different
sequences as output.  When you look at the algorithms in the standard library,
you will find that those that produce sequences as output do so by taking an
output iterator as an argument, and writing the resulting sequence to it.

My main problem with this was that the storage for the sequence must exist. If I
want to break down a string of code units into graphemes, I need to decode said
string, and apply the grapheme segmentation rules given by the Unicode Standard
to the result of that.

Following the approach used in the standard library, I would end up with code
similar to the following.

{% highlight cpp %}
std::vector<ogonek::code_point> decoded;
ogonek::decode<encoding>(source.begin(), source.end(), std::back_inserter(decoded));
std::vector<ogonek::grapheme> graphemes;
ogonek::graphemes(decoded.begin(), decoded.end(), std::back_inserter(graphemes));
{% endhighlight %}

Note how the return values of the functions go completely unused, and how you
end up having to store a decoded version of your data before you use the
grapheme segmentation algorithm, even if you won't make use of it. I don't like
that limitation; I am a sucker for lazy evaluation and would rather have the
decode operation be chained with the grapheme grouping without intermediate
storage.

One alternative would be to provide a grapheme segmentation algorithm
that works with a specific encoding, or a different one for each encoding.
I don't like that idea because it is a bit limiting.

Another problem with iterators is that in general an iterator is not usable by
itself.  You need two of them so you know when to stop. That means that if you
want to return a lazy sequence, you cannot simply return an iterator for it: you
would need to return a `std::pair` of them.

Returning a pair is still not good because it's not a composable scheme:
standard algorithms do take pairs of iterators, but as separate arguments, not
as a single `std::pair` argument.

### Boost.Range

So I peered into Boost and Boost peered back with [Boost.Range]. People told me
about how cool it was with the chaining `operator|` thing and so on. I gave it a
try and designed interfaces based on it.

After using it for a while I realised that Boost.Range wasn't really helping
much. It provides a nice-looking interface for some operations, but it falls
very short as soon as one needs something non-trivial.

#### Iterators all over again

One of the biggest gripes I had with Boost.Range was that when you look at it,
Boost.Range is just iterators all over again. Boost.Range is easy to use if you
are merely consuming ranges and range adaptors, but doesn't help at all if you
want to create some yourself: you have to go back to writing your own iterators.

[Boost.Iterator] helps a bit in writing your own iterators, mostly because it
takes all the boilerplate with typedefs and all the various operators away, but
I think it doesn't help enough.

#### You can never have enough end iterators

One issue I found repeatedly relates to writing iterator adaptors that need to
read an unknown number of elements from the underlying sequence to produce one
element on the adapted sequence. The most obvious example of this is a decoding
iterator.

Since an iterator on its own does not know when the sequence ends, the decoding
iterator cannot just wrap one iterator from the sequence being decoded. Say,
when decoding UTF-8, if we find the start of a three-byte sequence, we need to
read two more bytes, and we need an end iterator to even know if those two other
bytes actually exist. So such an iterator is forced to carry around not only the
iterator it is wrapping, but the end iterator as well.

This gets awkward pretty fast. Think about an end iterator for the adapted
sequence. Since it has the same type as a regular adapted iterator, it carries
around two iterators, and they are both the end iterator of the underlying
sequence.

Returning such an adapted range looks something like the following:

{% highlight cpp %}
return boost::make_iterator_range(
    make_adapted_iterator(s.begin(), s.end()),
    make_adapted_iterator(s.end(), s.end()));
{% endhighlight %}

And now consider what happens if you adapt one of those iterators with another
iterator adaptor that suffers from the same awkwardness. Each of these new
iterators will hold a pair of the adaptors on the first level, each of which has
a pair of iterators from the source sequence. All of a sudden you are carrying
around eight iterators from the source sequence, but you are only actually
keeping track of two positions in that sequence (seven of those eight iterators
are end iterators). This exponential increase in size for no gain makes me
uneasy.

### The wheel

Eventually all these issues reached a point where I felt like I was fighting the
abstraction too often, so I went back to the drawing board. I drew ideas from
the [D standard library][phobos], from .NET's [IEnumerable] interface, from my
experience with [Haskell][Data.List], and from Bart de Smet's [MinLINQ]
experiment.

The result of this will be the subject of my next post.

 [ogonek]: http://github.com/rmartinho/ogonek
 [Boost.Range]: http://www.boost.org/libs/range/doc/html/index.html
 [Boost.Iterator]: http://www.boost.org/libs/iterator/doc/index.html
 [taussig]: http://github.com/rmartinho/taussig
 [phobos]: http://dlang.org/phobos/std_range.html
 [IEnumerable]: http://msdn.microsoft.com/en-us/library/9eekhta0.aspx
 [Data.List]: http://www.haskell.org/ghc/docs/latest/html/libraries/base/Data-List.html
 [MinLINQ]: http://bartdesmet.net/blogs/bart/archive/2010/01/01/the-essence-of-linq-minlinq.aspx
