---
layout: post
title: Size matters, part 3
categories: cxx11
short: where I abuse overload resolution for fun and profit
---

In [part 2][previous] we got our types sorted properly for optimal layout. The
next step is building the mappings to and from the storage indices.

Before we start, I need to make clear what these mappings are. The code can be
quite confusing and difficult to follow without a clear idea of this.

When we have a call like `get<0>(some_tuple)` we need to know where in the
storage is the element 0, since we may have relocated it to get our optimal
layout. In the picture below, we can see an example where the element 0 is at
position 1 in storage. We need to map index 0 from the interface to index 1 in
the storage.

![Mapping from interface to storage][mapping1]

That would mean that our representation of this map, being merely a list of
indices, would store in position 0 the value 1: `indices<1, x, y, z>`. Applying
the same logic to the other elements, we would have `indices<1,3,2,0>`. 

This map is not enough, though. If we have a call to `make_tuple(a, b, c, d)`,
we need to rearrange these arguments to place each one at the appropriate spot.
To call the constructor for the tuple that we use for storage, we need to figure
out which of those arguments to place at index 0, so we can pass it as the first
argument of that constructor.

![Mapping from storage to interface][mapping2]

The picture shows that at index 0 we would place the object that was given at
position 3. The previous map does not readily provide this information. We need
one that has a 3 at position 0, and so on for the other elements. For this
example, that map would be `indices<3,0,2,1>`.

### Mapping from storage to interface

After sorting, we have type list that might look like the following.

{% highlight cpp %}
using sorted_example = std::tuple<
    indexed<layout<1>, 1>
    indexed<layout<2>, 3>,
    indexed<layout<4>, 2>,
    indexed<layout<4>, 0>,
>
{% endhighlight %}

This type contains information about the order of the types, which we will need
to define a `std::tuple` for storage; and also the indices for the map from the
storage indices to the interface indices, which we will need to write
`make_tuple`. We just need to extract this information into a more appropriate
format.

So far we needed to carry the types and indices together, and now we need to
split them. Variadic templates actually make this task pretty easy. We can
simply do some pattern matching in a partial specialization and take the two
parameter packs at once. Then we repack them separately in two different types.

{% highlight cpp %}
template <typename List>
struct split;
template <typename... T, std::size_t... I>
struct split<std::tuple<indexed<T, I>...>> {
    using tuple = std::tuple<T...>;
    using map = indices<I...>;
};
template <typename List>
class optimal_order {
    using sorted = Sort<WithIndices<List>>;
    using tuple = typename split<sorted>::tuple;
    using to_interface = typename split<sorted>::map;
    //using to_storage = ...;
};

template <typename... T>
using Storage = typename optimal_order<std::tuple<T...>>::tuple;

template <typename... T>
using MapToInterface =
typename optimal_order<std::tuple<T...>>::to_interface;

template <typename... T>
using MapToStorage =
typename optimal_order<std::tuple<T...>>::to_storage;
{% endhighlight %}

One map down, one to go. The other map is a bit trickier. We could implement it
by simply searching for the position of each index in the map we already built.
A naive approach would result in O(N<sup>2</sup>) template instantiations, but
we can avoid that and keep everything compiling with linear instantiations.

### Convincing overload resolution to do our work for us

Instead of writing a template to search for indices, we can use overload
resolution to do that search for us. When we pass a type that inherits from
multiple bases into a function with different overloads for each base, the
compiler considers all those overloads as possible for the call. If those
overloads are template functions, we can narrow down the possibilities by making
some template parameters explicit. We can take advantage of this to do our
search and don't need to write our own with recursive template instantiations.
         
For this to work, we need a type that derives from all the candidates for our
search. That is not difficult to produce using variadic templates.

{% highlight cpp %}
template <typename List>
struct inherit_all;
template <typename... T>
struct inherit_all<std::tuple<T...>> : T... {};
{% endhighlight %}

We will use only our empty TMP types, so we can inherit from them directly. If
there was a possibly of having fundamental types, references, etc, we would need
some sort of wrapper because we cannot inherit from those.

I should also note that there is no risk for ambiguity here: we know for certain
that all types are different specializations of `indexed` and each one has a
different index.

Now we need a function template that accepts all our candidates. Those
candidates need to carry around their positions in the map we have already
computed. We can use our `indexed` and `WithIndices` templates for this.

{% highlight cpp %}
template <typename T, std::size_t I, std::size_t J>
using indexed2 = indexed<indexed<T, I>, J>;
{% endhighlight %}

This parameter of our function template will be built from this template. It
actually has information we don't need (the type), but that is not problematic,
and it has the advantage that it can be built simply by attaching indices to our
already sorted list.

{% highlight cpp %}
template <std::size_t Target, std::size_t Result, typename T>
index<Result> find_index_impl(indexed2<T, Target, Result> const&);
{% endhighlight %}

Note how the index we are looking for is the first template parameter. This is
so we can pass it explicitly, and still have the compiler deduce the rest. The
type deduction is crucial here: it is how we get our result back. We put that
result into the return type so we can obtain it easily with `decltype`.

{% highlight cpp %}
template <std::size_t Target, typename List>
struct find_index
: decltype(find_index_impl<Target>(inherit_all<List>{})) {};
{% endhighlight %}

With this we can write `find_index<0, sorted_example>::value` and obtain 3, and
we have a constant number of template instantiations.

### Putting it all together

To put everything together we need to apply `find_index` to all indices from 0
to N-1, where N is the number of types involved. This can be done easily if we
have a `IndicesFor` alias that builds a list of indices with the same size as a
given tuple. I will leave that as an exercise for the reader.

{% highlight cpp %}
template <typename List, typename Indices = IndicesFor<List>>
struct map_to_storage;
template <typename List, std::size_t... I>
struct map_to_storage<List, indices<I...>>
: identity<indices<find_index<I, List>::value...>> {};

template <typename List>
class optimal_order {
    //using sorted = ...;
    // ...
    using to_storage = typename map_to_storage<WithIndices<sorted>>::type;
};
{% endhighlight %}

### The lower level

And we finally have enough machinery in place to start the actual tuple class.
So far everything was done on the metaprogramming level, but the next post in
this series will finally involve a bit at the "regular" program level.

 [mapping1]: /images/2012-12-09-optimal-tuple-iii-01.png 
 [mapping2]: /images/2012-12-09-optimal-tuple-iii-02.png 

 [previous]: /cxx11/2012/12/02/optimal-tuple-ii.html "Previously..."
<!-- [next]: /cxx11/2012/12/16/optimal-tuple-iv.html "To be continued..." -->

