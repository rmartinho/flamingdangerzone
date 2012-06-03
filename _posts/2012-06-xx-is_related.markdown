---
layout: post
title: Some pitfalls with forwarding constructors
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
    optional(U&&...);

    // copy constructor
    optional(optional<T> const&);

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
};
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

 [boost-optional]: http://www.boost.org/libs/optional/index.html 

