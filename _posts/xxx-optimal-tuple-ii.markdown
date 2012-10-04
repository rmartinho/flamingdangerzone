---
layout: post
title: Size matters, part 2
---

On [part 1][previous] we saw how two common `std::tuple` implementations could
result in a sub-optimal layout, and how an optimal layout could be achieved.

Now we'll see what an implementation of a tuple with such an optimal layout
actually looks like.

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

 - sort the elements to find out the optimal storage order;
 - maintain a map of their position in the template parameter list to their
   position in storage, so that `std::get` works;
 - maintain a map of their position in storage to their position in the template
   parameter list, so that `std::make_tuple` works.

And how should we keep track of these maps? We're going to need some sort of
compile-time list. Some template that can have an arbitrary number of
parameters, and that has metafunctions to get them by index. Something like...
`std::tuple`! A `std::tuple` of `std::integral_constant`s works nicely as a
compile-time list of integers. Some aliases may make it easier to work with
these though.

    template <std::size_t I, typename T>
    using TupleElement = typename std::tuple_element<I, T>::type;
    template <std::size_t I>
    using index = std::integral_constant<std::size_t, I>;
    template <std::size_t... I>
    using indices = std::tuple<Index<I>...>;

As an example, the mappings for `my::tuple<layout<1>, layout<4>, layout<2>>`, on
libc++'s implementation would be `indices<2, 0, 1>` and `indices<1, 2, 0>`.

![Mappings in action][mappings]

### The sorting of the tuples

 [mappings]: /images/2012-07-09-optimal-tuple-ii-01.png

 [previous]: /2012/07/06/optimal-tuple-i.html "Previously..."
<!-- [next]: /2012/07/13/optimal-tuple-ii.html "To be continued..." -->

