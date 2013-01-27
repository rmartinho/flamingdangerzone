---
layout: post
title: Even more type traits
categories: cxx11
short: where I showcase more cool traits
published: false
---

I have before talked about some type traits I use often. I feel like there are
more type manipulations that I want to share.

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

I find that more convenient to both write and read. Additionally, it can also be
used if you don't want to write an alias for a single use.

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

 [handling dependent names]: /cxx11/2012/05/27/dependent-names-bliss.html
 [bare types]: /cxx11/2012/05/29/type-traits-galore.html#bare_types
