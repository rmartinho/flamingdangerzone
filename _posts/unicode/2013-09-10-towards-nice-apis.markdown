---
layout: post
title: The little tail that could
categories: [unicode, cxx11]
short: where I go over ogonek's API design
last_edit: 9 September 2014
published: true
---

For the past year or so, I have been dedicating some of my free time to the
design and implementation of a C++11 library for dealing with Unicode, that I
decided to dub ['ogonek'][ogonek].

 [ogonek]: http://github.com/rmartinho/ogonek

Now, for some background, I have a very poor opinion of many C++ APIs out there.
Boost is pretty much the only one that I hold in high regard in that
respect&mdash;even though I do have my complaints about it.

For my own API, the first things I set in stone were the design goals and a few
golden rules to produce an API "the way I like it". I feel like at least the
first one should be the rule everywhere, but as I learned after all this time,
it is not.

I am following them in my interface design, and listing them here for others to
get an understanding of why some things are the way they are, and maybe to adapt
them for their own designs.

## The pit of success

I really like the pit of success/pit of despair analogy. Some languages/APIs
make it really easy for you to mess up&mdash;a small mistake and you fall into
the pit of despair. They require a lot of care to do things right. You are
basically walking on the border of the pit of despair, and once you fall in, it
takes effort to get out of it.

Some other languages/APIs make it hard to mess up and instead lead you naturally
to good solutions. This is called "pushing you into the pit of success".  If you
want to do something that is questionable and dangerous with one of these
languages/APIs, you need to work for and climb out of the pit of success.

As should be obvious by now, I have a strong dislike for pit of despair APIs,
and a strong preference for pit of success APIs. Ogonek's API is designed to be
a pit of success API.

Validity and correctnesss come first; speed is irrelevant in the face of those.
But don't get me wrong&mdash;none of this means that I am "designing for slow".
Efficiency is still a valid concern, and I have indeed made several design
decisions that stem from a need for efficiency, but always within the bounds of
correctness.

In the end, I am striving for having my cake and eating it too: I want a design
that favours correctness and allows efficient implementations. That may be a lot
to ask for, but I believe I can achieve such a design that enforces correctness
and allows efficiency enough for many tasks.

In few words, whenever the choice between a correct design and an efficient
design must be made, I pick the correct design. Whenever I can pick both, I pick
both.

As you will see from the rest of this article, this pit of success idea is the
overarching theme of most decisions I made in the API and is highly pervasive
throughout it.

## Explicit is better than implicit

If you care about correctness, you don't want all kinds of stuff happening
behind your back as that makes it harder to keep track of what is happening and
makes it easier for mistakes to go unnoticed.

Some people prefer "convenience" APIs that sort of guess what you meant to do
and if they guess wrong you get to debug that later. Oh, the joy. Needless to
say, I don't like those.

Ogonek won't be like that&mdash;it will make no guesses as to what you meant.
Don't expect the library to make any arbitrary decisions in your name&mdash;if
you don't care about some details important for correctness, ogonek is not
suited for you.

I know this might make the API harder to learn, because it will require some
people to be aware of certain things that they never came across before. I think
this is a good thing. It means that if you try to do something without a minimal
understanding of it, the code will likely not compile and you will be forced to
go and grab some fundamental domain knowledge. Once again, it's the pit of
success idea at work.

## Implicit is better than explicit

At this point you may be thinking if this heading is an editorial mistake. It
isn't. While I argued for making some operations explicit, I don't have a fetish
for unnecessarily verbose code. There are many operations that can be performed
implicitly because they aren't lossy, surprising, dangerous, or unreasonably
inefficient.

These operations that can be implicit are especially common if you follow the
pit of success rule in designing the API as there will be less ways of truly
messing up. And with this the API ends pushing you even harder into the pit of
success, since it gives you less surface area to make mistakes.

In those operations there is usually no choice to be made by the user, and
forcing them to explicitly choose the single alternative available is annoying
and only adds clutter to the code.

In short, operations on ogonek are automated whenever that does not reduce
flexibility or functionality, and require explicit intervention otherwise.

## Fail fast, fail loudly

So what happens whenever the user makes a mistake? Well, something will fail,
sometime. There are several possible times for that failure to manifest itself.
For what concerns API design, the relevant possibilities are, in chronological
order, as follows.

**Compilation time**&mdash;some mistakes can be detected by the compiler.  The
most common of such mistakes are either syntactic errors or type errors.
Syntactic errors don't have much influence in API design, but type errors are
very important. The more information that is encoded in types, the more things
can be checked automatically by the compiler.

**Linkage time**&mdash;these are either boring or really annoying to fix. There
used to be a common idiom in C++03&mdash;private copy constructors without a
definition&mdash;that would cause some rare mistakes to manifest themselves at
linkage time, but I don't think there is anything useful about this kind of
errors for driving modern API design or implementation.

**Testing time**&mdash;this is when violated assertions manifest themselves.
Some assertions will always be hit when wrong code is used, and as usual
produce a nasty error message in some test log. Those are very useful to have,
because they point out bad code as soon as it runs, every time it runs. While
not explicitly part of the language-level interface, those assertions do form
part of the effective interface of the library, and they serve to enforce it
beyond what is possible to enforce through type-checking.

**Run time**&mdash;the difference between this and testing time is subtle. Some
mistakes won't manifest at any previous stage, and will manifest under very
unique or unusual circumstances. Unless you are lucky, these can slip through
testing and end up in the final product, undetected until some user finds it
and hopefully files a bug report.

The longer an error is allowed to remain, the more code will be built around it,
and that increases the likelihood of a fix affecting a large portion of other
code.

In ogonek I strive to have as many errors as possible manifest themselves upon
compilation. This means encoding more and more information in types. This
certainly involves some template meta-programming, but so far always in healthy
doses.

A very important property that is encoded in some ogonek types is whether they
represent a well-formed sequence. All externally provided sequences are
considered of unknown well-formedness. Some processes, like validating or
decoding, accept such sequences and return sequences that are thus statically
known well-formed. Some functions accept only well-formed sequences as input,
and using them with sequences of types that have not been blessed by the
well-formedness gods will cause a compiler error&mdash;hopefully I can make it
a descriptive one&mdash;and force you to think about whether your input is
known to be valid or not.

{% highlight cpp %}
std::u16string not_known_wellformed = f(); // u16string could come from anywhere
ogonek::text<utf8> known_wellformed = g(); // text has well-formedness invariant

auto x = ogonek::titlecase(not_known_wellformed); // error
auto y = ogonek::titlecase(known_wellformed); // fine
auto z = ogonek::titlecase(ogonek::decode<utf16>(not_known_wellformed)); // fine
{% endhighlight %}

As a side effect of this, I can do some efficiency improvements. Some operations
accept sequences regardless of their well-formedness, like initialising an
instance of ogonek's core string type (named `text`). Having information about
well-formedness available statically, I can make a validation function that is a
no-op when the input is known well-formed, and performs regular validation
otherwise. I really like this because I get both correctness and efficiency from
this design.

I also keep track of other properties statically in a similar manner, in order
to enforce some other invariants and enable some other optimisations, but
well-formedness is the most important one.

## You are not alone

One thing that really annoys me when I look at a C++ API is seeing a complete
disregard for interoperation with the standard library. An API designed in a
vacuum will cause you pain when you need to use it with other code. If it cannot
work with the standard library that is really annoying, because the standard
library is used everywhere (it is, isn't it?).

I want ogonek to integrate nicely with the existing language features and with
the existing standard library. That means supporting the use with standard
algorithms and containers by providing and consuming iterators, and supporting
the use of iostreams.

## Escape hatches

So, ogonek will toss you into this sanitised world where you can only do things
that won't hurt you. However, sometimes you really want to walk on the edge of
the pit, instead of jumping straight into it.

Maybe you don't want to validate that string you obtained from that library as
you are sure is well-formed, even though the library provided it as some old
dusty type that won't be considered well-formed by ogonek. Maybe you want to
access the underlying storage of that string and mess directly with the bytes
and not the code points. *"I know what I am doing."* you say, hoping those won't
be your last words.

Well, if you want to walk on the edge, I can allow that. The C++ ecosystem has
been around for a long time before ogonek, and sometimes you really need to get
out of the sweet and cozy safety bubble to interoperate with some old code, or
to implement some algorithm in the most performant manner. Ogonek caters for
this by providing escape hatches for most safety mechanisms.

If you have a sequence that you know is well-formed because, say, it was
returned by a library that never produces ill-formed sequences, you can simply
mark it as well-formed without actually performing the validation. Remember
though, that you are walking on the edge here. This is functionality that can be
misused, and when that happens, there is nothing the library can do to help.

{% highlight cpp %}
std::string legacy_well_formed = some_well_behaved_function();
// This is cheaper than validating, but dangerous; only safe because our data
// came from a function that only produces well-formed data (let's hope it's not buggy)
auto decoded = ogonek::decode<utf8>(legacy_well_formed, ogonek::assume_valid);

std::string not_always_well_formed = some_ill_behaved_function();
// This is an advanced dangerous feature; the following would result in
// undefined behaviour
//auto decoded = ogonek::decode<utf8>(not_always_well_formed, ogonek::assume_valid);
// The simplest solution is still safe because it validates the input:
auto decoded = ogonek::decode<utf8>(not_always_well_formed);
{% endhighlight %}

Some other escape hatches are more like airlocks, though. You can get out of the
safety bubble, but if you want back in, you need to go through decontamination
in the airlock.

As an example of such a mechanism, imagine you want to access the underlying
storage of the `text` type in ogonek, in order to, for example pass some bytes
directly to a legacy function. The normal interface for the string exposes code
points, but many legacy functions out there take bytes in some known encoding
directly.

{% highlight cpp %}
void legacy_function(std::string const& utf8_bytes);
void legacy_mutating_function(std::string& utf8_bytes);
{% endhighlight %}

In this case, you have two options. If you want the underlying storage merely to
look at it, you can simply obtain a `const` view of it, but if you want to
perform some mutation on that storage, you cannot do that in-place&mdash;that
would allow you to violate one of the invariants of the string type, namely the
one that says the storage is always well-formed.

{% highlight cpp %}
ogonek::text<utf8> t = g();
legacy_function(t.storage()); // const view
//legacy_mutating_function(t.storage()); // error: it's const
{% endhighlight %}

In order to achieve such direct mutation of the storage without breaking
invariants, the only way to get a mutable view of the storage is to move it out
of the `text` object.  That leaves an empty text&mdash;empty text still
holds the invariants&mdash;and the old storage moved into an external object.
That object can be modified in any way desired, and after that it can be
reassigned to the old text that was empty, or used to initialise a new one. This
reassignment or initialisation is the decontamination procedure. It enforces the
`text` invariants by validating the storage again for well-formedness. As you
may have guessed, you can also escape this validation if you are sure the
mutation didn't produce an ill-formed sequence by using the aforementioned
validation escape hatch.

{% highlight cpp %}
std::string s { t.extract_storage(); }; // take storage out
assert(t.empty()); // storage was moved out
legacy_mutating_function(s); // mutate the storage
t = std::move(s); // move storage back in, validation performed
//t.assign(std::move(s), ogonek::assume_valid); // just a move, no validation
{% endhighlight %}

Altogether, I really like the direction that these few rules drove the API
towards.  It takes some extra effort to implement, but in the end I get an API
that has many, many correctness checks made by the compiler, and still keeps
reasonable efficiency.

I can have my cake and eat a significant portion of it.

