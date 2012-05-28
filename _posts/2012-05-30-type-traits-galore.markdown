---
layout: post
title: <type_traits> extended
---

The C++11 standard library provides us with several type traits, many of which
need compiler magic and thus cannot be reproduced within the language. These
type traits are very useful, but, at least for me, they still don't cover a lot
of common uses. This article will cover some of these useful traits that are not
found in the standard library.

### Identity metafunction

The first one is the simplest one. An identity metafunction is one that returns
its argument unchanged. It's the most basic type transformation: it performs no
transformation at all. The implementation is rather simple as well.

{% highlight cpp %}
    template <typename T>
    struct identity { using type = T; };
{% endhighlight %}

This metafunction already [exists in the Boost.MPL library][mpl-identity], but
since it is so simple, I don't mind writing it myself.

But, what is a "do-nothing" construct useful for?

Well, it is useful when we need to do something, but want to do nothing! An
example is when we use `std::conditional` and the branches are evaluated lazily.
If one of the branches is a known type instead of a type computation, we need to
make a no-op type computation out of it.

{% highlight cpp %}
    template <typename T>
    struct foo {
        using type = std::conditional<
            std::is_something<T>::value,
            identity<T>, // produce a T lazily
            foo<typename change<T>::type>
        >::type::type;
    }
{% endhighlight %}

### Identity alias

We can define an identity alias that "invokes" `identity` by accessing its
`type` member.

{% highlight cpp %}
    template <typename T>
    using Identity = typename identity<T>::type;
{% endhighlight %}

Is this one as useful as the other one?  Why not simply write the type directly
instead of wrapping it in `Identity`?

There is one useful property of this alias: since dependent names cannot take
part in type deduction, we can use this alias to prevent type deduction. This
isn't very common, but it does pop up sometimes. In the standard library we have
`std::forward` as an example of a function where type deduction is not desired.
Since this is the only meaningful use case for this alias, I prefer to give it a
more appropriate name:

{% highlight cpp %}
    template <typename T>
    void no_deduction(NotDeducible<T> x);
{% endhighlight %}

And what if we wrote this alias directly without invoking `identity`?

{% highlight cpp %}
    template <typename T>
    using Alias = T;
{% endhighlight %}

This is truly mostly useless. I only know two use cases for it, and they're both
shady.

We can use it to [create a temporary array][temporary-array], but who would want
that?

We can also use it to work around the quirks of the C declarator syntax:

{% highlight cpp %}
    void f(alias<int[10]>& a);
    // instead of void f(int(&a)[10]>);

    alias<int[10]>& f();
    // instead of int (&f())[10]
{% endhighlight %}

And this is already too much talking about nothing.

### Dependent boolean

`static_assert` is a new language feature that allows us to produce errors when
a certain compile-time boolean expression is false. One use case of this feature
is providing custom error messages when using templates.

{% highlight cpp %}
{% endhighlight %}

 [mpl-identity]: http://www.boost.org/doc/libs/release/libs/mpl/doc/refmanual/identity.html
 [temporary-array]: http://stackoverflow.com/a/10624677/46642

