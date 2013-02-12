---
layout: post
title: Even more type traits
categories: cxx11
short: where I showcase more cool traits
published: false
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
to call those [bare types]. After trying the name on some people and after
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
appropriate size and alignment for some type.

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
template <typename... T>
using StorageFor = Invoke<std::aligned_storage<sizeof(T), alignof(T)>>;
{% endhighlight %}

I will expand on this trait latter when I explore the use cases and show
improvements to it.

