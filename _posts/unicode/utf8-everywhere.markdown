---
layout: default
title: How I want to use Unicode in C++
---

## UTF-8 everywhere is a hack

I think that using [UTF-8 everywhere][manifesto] is nothing but a hack. Sure it
may work if you put up with it, but it is a sub-optimal choice.

As an engineer, I prefer to use *the right tool for the job*. Hammers are all
nice and dandy if I'm nailing someone to a cross, but please keep them away from
my screws.

### Right tool for the job

Different encodings have different characteristics, and there are trade-offs
involved when picking one. Do you think that you should always make the same
trade-offs in all situations? I don't.

UTF-8 gives you ASCII compatibility, uses less space in some common cases, and
is the encoding used in some common network protocols.

UTF-16 gives you cheap interoperation with some systems like Windows, .NET, or
Java.

UTF-32 gives you a cheap encoding-agnostic interface: you just treat the data
directly as codepoints and not as code units.

UTF-8 may be suitable for serialization scenarios like storage and transmission
over the wire, because UTF-8 often uses less space than the other UTF encodings.

UTF-32 may be suitable for some text manipulations, because it blurs the line
between codepoints and code units. You pay for that convenience with some wasted
space, so you probably don't want to use it for, say, sending text over the
wire.

I cannot think of any reason to use UTF-16 other than interoperating with systems
that made the mistake of using UTF-16 everywhere.

### Choices, choices everywhere!

Encoded in UTF-32, the King James version of the Bible uses a whopping sixteen
megabytes of memory. Now consider how much text your application is manipulating
when compared to the KJV Bible. Then consider on how many systems your
application runs where sixteen megabytes of text are too much to handle. For me
this means that UTF-8's space savings may not be worth giving up the
conveniences of UTF-32 for text that lives in RAM. Most applications are not the
Library of Congress (and if they were, would they keep the whole thing in memory
all the time?)

Yes, I'm wasting twelve megabytes with that bible, but I have an entire freaking
Bible in memory. For permanent storage of said Bible, or for sending it over the
wire, I might prefer something like UTF-8. However, since I am the one making a
choice and storage/bandwidth is so limited, why bother with UTF-8 at all? Why
not go directly to one of the compression schemes that exist for Unicode (SCSU
and BOCU-1)?

This is not to say that there are no more options or that there is a best
option. There is no One Ring to rule them all and in the darkness bind them.

This is to say that there are choices involving trade-offs to be made. An
engineer will pick the most appropriate trade-off for the situation at hand; a
fashionista will pick whatever the latest trend is.

But this hammer-screw argument I just made is not the reason I don't like
http://utf8everywhere.org.

### Text is text is text

The reason I don't like http://utf8everywhere.org is that it focuses on
the wrong details. I believe the encoding is irrelevant for most text-related
tasks. When I am manipulating text, I want to deal with text; I want to care
about characters, words, sentences, lines; I don't want to care about
continuation bytes, or about surrogates. *What kind of masochist wants to care
about that?*

What is an encoding but a serialization format? If I'm not serializing my text,
why am I dealing directly with a serialization format? Is a sentence any
different because it is encoded as UTF-8? It is not!

As if text were not complex enough when made of Unicode codepoints, you are
telling me that I should use text made of code units? No!  That simply does not
make any sense for me.

### One does not simply use a string

No, don't give me `std::string`. `std::string` is nothing but a `vector<char>`
that keeps an extra null character at the end and has an enormous interface that
would be much better living in `<algorithm>`. How helpful is an array of bytes
when dealing with text? Text is not bytes.

I want my text manipulation interfaces to be, for the most part, agnostic of the
encoding that is used to store the text. If I have that, I can use whatever
encoding has trade-offs that are appropriate for my problem; I can use the right
tool for the job.

But I want more.

### Use ALL the encodings!

I want the interfaces to be agnostic of encoding but I don't want the encoding
to be chosen for me. I want to choose the right tool for the job. I want an
interface that allows me to pick the right encoding, but does not make me work
in terms of it.

I want the interfaces to provide me with the tools for *serializing and
deserializing text* to and from whatever other encodings I might need. The right
tool for one job is not the right tool for all jobs.

### Invariants or GTFO

I want invariants. I don't want `char*`, aka "a bunch of bytes that may or may not be
actual text". I want the interface to give me the guarantee that text is text. I
don't want to have to test if some piece of text is a valid sequence of code
units, because I don't want to deal with code units. If I read in some lone
surrogates I cannot create text from it; it follows that I should not find
myself with an object representing that nonexistent text.

### Am I living in the land of ponies and Unicodes?

Does this magical interface exist? Am I the only one that is fed up with the
Jabberwocks that go by the names "UTF-8 text" and "UTF-16 text"? Can haz vorpal
sword?

  [manifesto]: http://utf8everywhere.org


