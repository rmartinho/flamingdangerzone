---
layout: post
title: Size matters, part 4
categories: cxx11
short: still don't know what this is about
published: false
---

Now that we can [map all indices back and forth][previous] between the interface
and the storage we can start fleshing out the tuple interface. For this exercise
I am going to provide exactly the same interface as the standard tuple class, so
that we can use this as a drop-in replacement.

### To member or not to member

As planned, our tuple will just delegate the actual storage to a standard tuple.
That will be the sole contents of our tuple. We could simply write something
like the following.

{% highlight cpp %}
template <typename... T>
struct tuple {
public:
    // interface goes here
private:
    OptimalStorage<T...> storage;
};
{% endhighlight %}

There is one thing I don't like about this though: it does not preserve some
properties of the underlying tuple, namely whether it is an empty type or not.
This implementation will never be an empty type, because it has a member. It
does not matter if that member is of an empty type.

{% highlight cpp %}
static_assert(std::is_empty<std::tuple<>>::value,
    "test is only meaningful if the standard tuple can be an empty type");
    // both libc++'s and libstdc++'s tuples are empty types when possible
static_assert(not std::is_empty<tuple<>>::value,
    "our tuple is not an empty type when it should");
{% endhighlight %}

This is a minor issue, but if we want to have a drop-in replacement, we want to
preserve as much as the original behaviour as possible.

We can preserve emptiness inheriting from the optimal storage instead. Since we
do not want to expose the storage, and do not want implicit conversions to mess
things up, we are going to inherit privately.

{% highlight cpp %}
template <typename... T>
struct tuple : private OptimalStorage<T...> {
private:
    // let's drop in a few typedefs while I'm at it ;)
    using storage_type = OptimalStorage<T...>;
    using to_interface = MapToInterface<T...>;
    using to_storage = MapToStorage<T...>;
public:
    // interface goes here
};
{% endhighlight %}

### The interface of a tuple

The [interface of a standard tuple][tuple reference] is pretty big, but there
are only a few members that require special attention in terms of
implementation. They vary from dead simple to pain in the mikta. If we take a
look at it we can see that most members are actually [constructors]. There's
very little to do with a tuple other than constructing and accessing elements.
Some important parts of the interface are provided as non-member functions
([`get`][get], three [factories], [relational operators], and
 [`tuple_cat`][tuple_cat]). And then we have the type traits like
[`std::tuple_element`][tuple_element] that we will want to specialize in the
`std` namespace.

Let's go through all of the members and non-members to get a general overview of
their implementations.

- The default constructor is dead simple: default constructing the storage works
fine, so the compiler generated one is ok;
- the two variadic constructors can delegated to the `std::tuple` constructors
by mapping from storage to interface;
- the copy and move constructors, as well as the copy and move assignment
operators can be compiler generated, since the underlying tuple already does all
the work;
- the constructors or assignment operators that convert from tuples with
different types can delegate to the `std::tuple` constructor using our map from
storage to interface indices;
- the uses-allocator constructors are similar to their respective regular
constructors;
- the factories can be simply implemented by delegating to the constructor of
the right tuple types; our tuple does not give a big advantage for `tie` and
`forward_as_tuple`, but implementing them is cheap anyway;
- `tuple_cat` is probably the most complicated of all; it will require some more
complicated mappings and will be the subject of a later post;
- `get` is to use our interface to storage mappings;
- everything else can be simply delegated to the underlying standard tuple.

As we can see, there a couple of trivial directly delegating implementations,
some that delegate with a map from interface to storage, some that delegate
with a map from storage to interface, and then there's `std::tuple_cat`.

 [tuple reference]: http://en.cppreference.com/w/cpp/utility/tuple "std::tuple reference"
 [constructors]: http://en.cppreference.com/w/cpp/utility/tuple/tuple "std::tuple constructors"
 [get]: http://en.cppreference.com/w/cpp/utility/tuple/get "std::get"
 [factories]: http://en.cppreference.com/w/cpp/utility/tuple/tie "e.g. std::tie"
 [relational operators]: http://en.cppreference.com/w/cpp/utility/tuple/operator_cmp "std::tuple relational operators"
 [tuple_cat]: http://en.cppreference.com/w/cpp/utility/tuple/tuple_cat "std::tuple_cat"
 [tuple_element]: http://en.cppreference.com/w/cpp/utility/tuple/tuple_element "std::tuple_element"

### Element access

Let's skip all the trivial stuff and start with the simplest non-trivial one:
`get`. There are three overloads, for different kinds of reference: `&`, `&&`,
and `const&`. This function will need access to the underlying tuple, so we will
declare it a friend of our tuple.

When we have a call like `get<I>(t)`, we need to map `I` to a storage index and
then just call the `std::get` on that index. For that we can use `tuple_element`
on the map we computed since our maps are actually tuples
[<sup id="ref1">1</sup>][ftn1]. That leaves us with the following
implementations.

{% highlight cpp %}
template <std::size_t I, typename... T>
using ToStorageIndex = TupleElement<I, MapToStorage<T...>>;

template <std::size_t I, typename... T>
TupleElement<I, tuple<T...>>& get(tuple<T...>& t) noexcept {
    return std::get<ToStorageIndex<I, T...>::value>(t);
    // std::get will access the private base; we are with friends here!
}
template <std::size_t I, typename... T>
TupleElement<I, tuple<T...>>&& get(tuple<T...>&& t) noexcept {
    return std::get<ToStorageIndex<I, T...>::value>(std::move(t));
    // we need to get an rvalue reference, so we use std::move
}
template <std::size_t I, typename... T>
TupleElement<I, tuple<T...>> const& get(tuple<T...> const& t) noexcept {
    return std::get<ToStorageIndex<I, T...>::value>(t);
}
{% endhighlight %}

Note how the return types don't need to map anything: we always want to use the
interface indices on the interface.

---

[1][ref1]<a id="ftn1"> </a>There is currently a bug in GCC 4.7.2 that causes
trouble for carrying a pack of indices as a `std::tuple`, so it may be necessary
to use a few workarounds. One alternative involves using a custom type to pack
the indices and a specialization of `std::tuple_element` for that type.

 [ftn1]: #ftn1
 [ref1]: #ref1

### Shuffle and forward

As should be obvious by now, we will need some way to forward a pack of
arguments appropriately shuffled according to one of our maps. This can be done
but expanding `get<I>` where `I` is a variadic pack of indices with our map.

 [previous]: /cxx11/2012/12/09/optimal-tuple-iii.html "Previously..."
 [next]: /cxx11/2012/12/23/optimal-tuple-v.html "To be continued..."

