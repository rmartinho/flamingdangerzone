---
layout: post
title: Some pitfalls with forwarding constructors
categories: cxx11
short: where I find that you can copy without invoking the copy constructor
last_edit: 05 June 2012
---

Let's say we're implementing a class template similar to
[`boost::optional`][boost-optional]. Unlike the one in boost, which was
implemented with C++03 features, we're adding a constructor that forwards
arguments to the constructor of the value type.

A naïve initial definition of that class template could look like this:

{% highlight cpp %}
template <typename T>
class optional {
public:
    // forwarding constructor
    template <typename... U>
    optional(U&&... u);

    // copy constructor
    optional(optional<T> const& that);

    // move constructor
    optional(optional<T>&& that);

    // rest of implementation omitted
};
{% endhighlight %}

The forwarding constructor has two issues: it allows implicit conversions from
any other type, and it is a copy constructor. Let's go over each one in turn.

### Implicit conversions

An implicit constructor with a variadic argument pack can be called with any
number of arguments. This means it can work as a default constructor if called
with zero arguments, or as a conversion constructor if called with one argument.
Because it is not `explicit`, this means it can be called to perform implicit
conversions. And it can accept *any* argument type! This isn't a very good idea.
It should be marked `explicit` to avoid accidental conversions.

{% highlight cpp %}
    template <typename... U>
    explicit optional(U&&...);
{% endhighlight %}

### Copy construction

How can that constructor work as a copy constructor if the class already has
one? Consider the following example:

{% highlight cpp %}
optional<int> o1;
optional<int> o2 { o1 };
{% endhighlight %}

Both the `optional<T> const&` constructor and the forwarding constructor are
candidates in this case. The forwarding constructor is a candidate because the
`U` parameter pack is deduced as a pack with `optional<T>&` as its single
element. And here is the problem: `optional<T>&` is a better match because it
doesn't require a conversion. Instead of making a copy, that code tries to
construct an `int` from an `optional<int>`.

How can this be solved?

### Enter SFINAE

We want to prevent the forwarding constructor from being selected by the
overload resolution process when we want the "normal" copy constructor instead.
We can achieve this by causing a substitution failure in that situation. All we
need is to specify a trait that detects that situation.

We want the copy constructor to be picked any time the type involved is
`optional<T>`, regardless of references or const-qualification. In other words,
any time the [bare type][bare types] involved is `optional<T>`.

{% highlight cpp %}
template <typename T, typename U>
struct is_related : std::is_same<Bare<T>, Bare<U>> {};
{% endhighlight %}

(I'm not very fond of the current name I use for this trait, but I haven't seen
a better one yet.)

Now we can use `DisableIf` to cause the substitution failure:

{% highlight cpp %}
    // forwarding constructor
    template <typename... U,
              DisableIf<is_related<U, optional<T>> // oops...
{% endhighlight %}

But there's a problem. `U` is a parameter pack. This raises issues: we only want
this check to be true if the pack has size one. If it has size zero substitution
will fail because `is_related` expects two arguments; this is undesirable. A
solution would be to have `is_related` take one type parameter and a type
parameter pack, along with a specialization for when two parameters are given.

{% highlight cpp %}
template <typename T, typename... U>
struct is_related : std::false_type {};

template <typename T, typename U>
struct is_related<T, U> : std::is_same<Bare<T>, Bare<U>> {};
{% endhighlight %}

And now the forwarding constructor can be written as follows:

{% highlight cpp %}
    // forwarding constructor
    template <typename... U,
              DisableIf<is_related<optional<T>, U...>>...>
    optional(U&&... u);
{% endhighlight %}

 [boost-optional]: http://www.boost.org/libs/optional/index.html 
 [bare types]: /cxx11/2012/05/29/type-traits-galore.html "More type traits"

