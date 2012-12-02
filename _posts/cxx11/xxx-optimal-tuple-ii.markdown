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
we call the maps respectively the *optimal map* and the *optimal reverse map*.
We need something that looks like this:

{% highlight cpp %}
    template <typename... T>
    class optimal_order;

    template <typename... T>
    using OptimalMap = typename optimal_order<T...>::map;
    template <typename... T>
    using OptimalReverseMap = typename optimal_order<T...>::reverse_map;
{% endhighlight %}

How can this be done? Let's start with something simpler: sorting the types by
their alignment. A simple insertion sort will do fine for a first
implementation.

#### Finding a predicate

First, we need to define a binary predicate that defines the order we are
looking for.

This order depends on how the layout of the underlying `std::tuple`. I'll be
leaving the detection of that for the build system. I think one might try to
detect it with some reasonable assumptions and a bit of TMP, but I'm leaving
that as an exercise for the reader.

Let's assume our build system defines of the macros
`MY_STD_TUPLE_LAYOUT_STRAIGHT` or `MY_STD_TUPLE_LAYOUT_REVERSED` according to
how the standard library being used does the layout. These macros can then be
used to decide to perform the actual comparisons with the arguments in reverse
order when appropriate.

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

This works because the compiler will have to layout `member<int&>` with the
proper alignment for a reference member, and we can grab that alignment from the
struct itself.

#### Carrying all the info

Now that we have a fine predicate, we can move on to sorting. The first thing we
need is to bundle all the information we need to carry around.  Just sorting the
types is not enough because we need to know what was their original index.

{% highlight cpp %}
    template <typename T, std::size_t I>
    struct indexed {
        using type = T;
        static constexpr auto index = I;
    };
{% endhighlight %}

For this to work we need a way to make a list of these `indexed` types from our
original list of types. So, let's say that we have an `WithIndices` alias that
produces such a list of indexed types, i.e., `WithIndices<int, double, int>`
produces `std::tuple<indexed<int, 0>, indexed<double, 1>, indexed<int, 2>>`. Now
we can sort this list.

#### Sorting with linear complexity

The types will be based on their alignment. One interesting property of those
alignments is that they are always powers of two. Not only this means there is a
finite and rather small number of possible values, but it also means that we
can know them all in advance.

Knowing this means we can write a sorting algorithm that does the sorting with a
linear number of template instantiations. That will certainly help with those
compilation times.

The algorithm can simply take all the types with each of the possible alignments
in the proper order of alignments. The order between types with the same
alignment is not relevant. This requires a number of passes equal to the number
of possible alignments, and each pass goes through the list once, i.e., is
linear.

To iterate over all the possible alignments we can compute the maximum alignment
first. That is something that is easily done with a little recursion with an
accumulator.

{% highlight cpp %}
    // max<N...> left as an exercise for the reader

    template <typename T>
    struct alignof_indexed;
    template <typename T, std::size_t I>
    struct alignof_indexed<indexed<T, I>> : std::alignment_of<member<T>> {};

    template <typename T>
    struct max_alignment;
    template <typename... T>
    struct max_alignment<std::tuple<T...>> : max<alignof_indexed<T>::value...> {};
{% endhighlight %}

Now, we need to iterate over the alignments in a different direction, depending
on how the standard library tuple we will be using behaves. We can use the
macros the build system defines for us and encapsulate this different behaviour
in a meta function. I think switching the iteration order requires more code
than instead building our result in reverse, so I am going to switch the
construction order of the result.

{% highlight cpp %}
    template <typename Head, typename Tail>
    struct cons;
    template <typename Head, typename... Tail>
    struct cons<Head, std::tuple<Tail...>>
    #if defined(MY_STD_TUPLE_LAYOUT_STRAIGHT)
    : identity<std::tuple<Tail..., Head>> {}; // append at end
    #elif defined(MY_STD_TUPLE_LAYOUT_REVERSED)
    : identity<std::tuple<Head, Tail...>> {}; // append at start
    #endif

    template <typename Head, typename Tail>
    using Cons = typename cons<Head, Tail>::type;
{% endhighlight %}

Next we can build a meta function that appends all types with a given alignment
from a given type list into another type list. Nothing harder than the usual
recursion over variadic parameter packs.

{% highlight cpp %}
    template <std::size_t Align, typename Acc, typename List>
    struct cons_alignment : identity<Acc> {};
    template <std::size_t Align, typename Acc, typename Head, typename... Tail>
    struct cons_alignment<Align, Acc, std::tuple<Head, Tail...>>
    : cons_alignment<
        Align,
        // append conditionally
        Conditional<Bool<alignof_indexed<Head>::value == Align>, Cons<Head, Acc>, Acc>,
        std::tuple<Tail...>> {};

    template <std::size_t Align, typename Acc, typename List>
    using ConsAlignment = typename cons_alignment<Align, Acc, List>::type;
{% endhighlight %}

And then sorting is simply a matter of iterating over all the alignments, down
from the maximum to 0. Since the alignments are powers of two, we just divide by
two to go down one alignment.

{% highlight cpp %}
    template <std::size_t Align, typename Acc, typename List>
    struct sort_impl
    : sort_impl<Align / 2, ConsAlignment<Align, Acc, List>, List> {};
    template <typename Acc, typename List>
    struct sort_impl<0, Acc, List> : identity<Acc> {};

    template <typename List>
    struct sort : sort_impl<max_alignment<List>::value, std::tuple<>, List> {};

    template <typename List>
    using Sort : typename sort<List>::type;
{% endhighlight %}

And now we can write `Sort<WithIndices<T...>>` and get the optimal map for a
list of types.

{% highlight cpp %}
    template <typename... T>
    class optimal_order {
        using map = Sort<WithIndices<T...>>;
        using reverse_map = /* on part 3 */;
    }
{% endhighlight %}

The next step is to figure out how to build the reversed map from this, which I
will explain in the next post on this series.

 [mappings]: /images/2012-10-20-optimal-tuple-ii-01.png 

 [previous]: /cxx11/2012/07/06/optimal-tuple-i.html "Previously..."
<!-- [next]: /xxxx/xx/xx/optimal-tuple-iii.html "To be continued..." -->

