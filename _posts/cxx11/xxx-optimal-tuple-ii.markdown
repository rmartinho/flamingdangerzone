---
layout: post
title: Size matters, part 2
categories: cxx11
short: where I sort types for optimal layout 
---

On [part 1][previous] we saw how two common `std::tuple` implementations could
result in a sub-optimal layout, and how an optimal layout could be achieved.

Now we'll see what an implementation of a tuple with such an optimal layout
actually looks like.

(Note: all this could have benefited from the Boost.MPL library, but I decided
not to use it in order to expose more ideas)

### Tuples all the way down

The main difficulty an implementation needs to overcome is the fact that the
user needs to treat the tuple as if its elements were in the order of the
template arguments: when a call to `get<0>` with a `tuple<int, double>` argument
is made, the result is the `int` even if the tuple was reordered to store the
`double` first.

The implementation will need to store the values internally in an optimal order,
but map to and from it in the function calls. And how should it store a
fixed-size collection of heterogeneous values? `std::tuple` seems like the most
obvious choice. The problem with doing that is that we need knowledge of the
implementation: libc++'s tuple lays out the elements in the order of the
template arguments, while libstdc++'s tuple lays them out in the reverse order.
Other implementations are certainly possible, but I think these two and one with
optimal layout are the only sane ones, so we won't catter for other
possibilities.

We could implement our own simple tuple for the underlying storage and then we
would know for sure what layout it used, but I decided to avoid reinventing that
wheel, at least for now.

So, the implementation needs to:

 1. sort the elements to find out the optimal storage order;
 2. maintain a map of their position in the template parameter list to their
    position in storage, so that `std::get` works;
 3. maintain a map of their position in storage to their position in the template
    parameter list, so that `std::make_tuple` works.

And how should we keep track of these maps? We're going to need some sort of
compile-time list. Some template that can have an arbitrary number of
parameters, and that has metafunctions to get them by index. Something like...
`std::tuple`! A `std::tuple` of `std::integral_constant`s works nicely as a
compile-time list of integers. Some aliases may make it easier to work with
these though.

{% highlight cpp %}
    template <std::size_t I, typename T>
    using TupleElement = typename std::tuple_element<I, T>::type;
    template <std::size_t I>
    using index = std::integral_constant<std::size_t, I>;
    template <std::size_t... I>
    using indices = std::tuple<index<I>...>;
{% endhighlight %}

As an example, the mappings for `my::tuple<layout<1>, layout<4>, layout<2>>`, on
libc++'s implementation would be `indices<2, 0, 1>` and `indices<1, 2, 0>`.

![Mappings in action][mappings]

### The sorting of the types

To define the optimal order, we need some metafunction that takes all types
involved and produces the two index maps. For ease of communication, let's say
we call the maps respectively the *optimal map* and the *reverse optimal map*.
We need something that looks like this:

{% highlight cpp %}
    template <typename... T>
    class optimal_order;

    template <typename... T>
    using OptimalMap = typename optimal_order<T...>::map;
    template <typename... T>
    using ReverseOptimalMap = typename optimal_order<T...>::reverse_map;
{% endhighlight %}

How can this be done? Let's start with something simpler: sorting the types by
their alignment. First, we need to define a binary predicate that defines the
order we are looking for.

This order depends on how the layout of the underlying `std::tuple`. I'll be
leaving the detection of that for the build system. I think one might try to
detect it with some reasonable assumptions and a bit of TMP, but I'm leaving
that as an exercise for the reader. Let's assume our build system defines of
the macros `MY_STD_TUPLE_LAYOUT_STRAIGHT` or `MY_STD_TUPLE_LAYOUT_REVERSED`
according to how the standard library being used does the layout. These macros
can then be used to decide to perform the actual comparisons with the arguments
in reverse order when appropriate.

{% highlight cpp %}
    template <typename T, typename U>
    struct layout_before_impl;

    struct layout_before
    #if defined(MY_STD_TUPLE_LAYOUT_STRAIGHT)
    : layout_before_impl<T,U> {};
    #elif defined(MY_STD_TUPLE_LAYOUT_REVERSED)
    : layout_before_impl<U,T> {};
    #endif
{% endhighlight %}

As we've seen on the previous post the optimal order places the members with the
strictest alignment first. We can use `alignof` to do this comparison.

{% highlight cpp %}
    template <typename T, typename U>
    struct layout_before_impl
    : std::integral_constant<bool, (alignof(T) > alignof(U))> {};
{% endhighlight %}

This implementation is rather simple, but there are a few issues with it.
One of them is that `alignof(int&)` does not give us the alignment that a
stored reference (as a member) will have, but instead it gives us the alignment
of `int`. This is not what we want if we have a tuple with references. Since we
really want the alignment of a reference member, let's simulate a member and
compute its alignment:

{% highlight cpp %}
    template <typename T>
    struct member { T _; };

    template <typename T, typename U>
    struct layout_before_impl
    : std::integral_constant<bool, (alignof(member<T>) > alignof(member<U>))> {};
{% endhighlight %}

{% highlight cpp %}
    template <typename T, typename U>
    struct layout_before_impl
    : std::integral_constant<bool,
        std::is_empty<T>::value
        || (!std::is_empty<U>::value && alignof(member<T>) > alignof(member<U>))
    > {};
{% endhighlight %}

 [mappings]: /images/2012-10-20-optimal-tuple-ii-01.png

 [previous]: /2012/07/06/optimal-tuple-i.html "Previously..."
<!-- [next]: /xxxx/xx/xx/optimal-tuple-iii.html "To be continued..." -->

