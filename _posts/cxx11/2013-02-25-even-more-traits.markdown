---
layout: post
title: Even more type traits
categories: cxx11
short: where I showcase more nice traits
last_edit: 10 March 2013
---

I have before talked about [some type traits][previous] I use often. I feel like
there are more type manipulations that I want to share.

 [previous]: /cxx11/2012/05/29/type-traits-galore.html#bare_types "Previously..."

### Metafunction invocation

One of my first posts here was about [handling dependent names]. I showed how
one can use alias templates to avoid having to type `typename ...::type` all
over. I actually went one step further and use a small alias to define those
aliases.

{% highlight cpp %}
template <typename T>
using Invoke = typename T::type;

template <typename T>
using UnderlyingType = Invoke<std::underlying_type<T>>;
{% endhighlight %}

I find that more convenient to both write and read even though it does very
little.

 [handling dependent names]: /cxx11/2012/05/27/dependent-names-bliss.html

### Unqualified types

I have talked about types stripped of qualifiers before. At the time I decided
to call those "[bare types]". After trying the name on some people and after
looking around for ideas, I have a found a much better and much more obvious
name. I now simply call these *unqualified types*. It is clear what I means, and
I no longer need to explain what "bare" means.

{% highlight cpp %}
template <typename T>
using Unqualified = RemoveCv<RemoveReference<T>>;
{% endhighlight %}

 [bare types]: /cxx11/2012/05/29/type-traits-galore.html#bare_types

### Raw storage

It does not happen often, but I have before needed to use raw storage of
appropriate size and alignment for some type (`boost::optional` without move
semantics, I am looking at you).

The C++11 standard library provides us with `std::aligned_storage` for this
task. It provides storage with a certain size and alignment.

However, I find the interfaces of this trait somewhat annoying. It is annoying
to use `sizeof` and `alignof` manually when all you want is "storage appropriate
for some `T`".

More than once have I used the type `std::aligned_storage<sizeof(T), alignof(T)>`
instead of `std::aligned_storage<sizeof(T), alignof(T)>::type` by mistake (I
even made that mistake in a previous post of mine). I have also seen other
people making that mistake on Stack Overflow.

So, obviously, I wrote a template alias to ease this.

{% highlight cpp %}
template <typename T>
using StorageFor = Invoke<std::aligned_storage<sizeof(T), alignof(T)>>;
{% endhighlight %}

I will expand on this trait latter when I explore the use cases and show
improvements to it.

### Improved decay

The standard library provides the `std::decay` trait to simulate pass-by-value
semantics. This is a transformation that shares similarities with the
`Unqualified` trait, but there are important differences: `std::decay` will
transform array and function types into pointer types, just like when passing
them by value.

`std::make_tuple`, for example, performs this kind of transformation. Using
`Unqualified` would not work for something like the following:

{% highlight cpp %}
void f();
auto tuple = std::make_tuple(0, f);
{% endhighlight %}

One cannot have members of function types. Decaying that type to a function
pointer does the natural thing and allows that to work. The result in the
example has type `std::tuple<int, void(*)()>`.

But `std::make_tuple` performs a slightly more involved transformation: in order
to allow creating tuples with references, one can use `std::reference_wrapper`
(through `std::ref` and `std::cref`).

{% highlight cpp %}
int x = 42;
auto tuple = std::make_tuple(0, std::ref(x));
{% endhighlight %}

This creates a `std::tuple<int, int&>` not a `std::tuple<int, std::reference_wrapper<int>>`.
`std::decay` cannot do that.

This kind of transformation is used in other places in the standard library as
well (`std::bind` for example), and comes up pretty often when writing generic
tools. So I think it's worth having a trait for it. I actually [used it][tuple iv]
in the tuple series to implement `make_tuple`.

{% highlight cpp %}
template <typename T>
struct unwrap_reference
: identity<T> {};

template <typename T>
struct unwrap_reference<std::reference_wrapper<T>>
: identity<T&> {};

template <typename T>
struct decay_reference : unwrap_reference<Decay<T>> {};
template <typename T>
using DecayReference = Invoke<decay_reference<T>>;
{% endhighlight %}

 [tuple iv]: /cxx11/2013/02/18/optimal-tuple-iv.html

