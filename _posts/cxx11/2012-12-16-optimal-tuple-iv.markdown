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
 [`tuple_cat`][tuple_cat]).

Let's go through all of the members and non-members to get a general overview of
their implementations.

- The default constructor is dead simple: default constructing the storage works
fine, so the compiler generated one is ok;
- the two variadic constructors require mapping from storage to interface, just
like `make_tuple`;
- the copy and move constructors, as well as the copy and move assignment
operators can be compiler generated, since the underlying tuple already does all
the work;
- the constructors or assignment operators that convert from tuples with
different types can use our map from storage to interface indices;
- the uses-allocator constructors are similar to their respective regular
constructors;
- the factories can be simply implemented by delegating to the constructor of
the right tuple types; our tuple does not give a big advantage for `tie` and
`forward_as_tuple`, but implementing them is cheap anyway;
- `tuple_cat` is probably the most complicated of all; it will require some more
complicated mappings and will be the subject of a later post;
- `get` is to use our interface to storage mappings;
- everything else can be simply delegated to the underlying standard tuple.

 [tuple reference]: http://en.cppreference.com/w/cpp/utility/tuple "std::tuple reference"
 [constructors]: http://en.cppreference.com/w/cpp/utility/tuple/tuple "std::tuple constructors"
 [get]: http://en.cppreference.com/w/cpp/utility/tuple/get "std::get"
 [factories]: http://en.cppreference.com/w/cpp/utility/tuple/tie "e.g. std::tie"
 [relational operators]: http://en.cppreference.com/w/cpp/utility/tuple/operator_cmp "std::tuple relational operators"
 [tuple_cat]: http://en.cppreference.com/w/cpp/utility/tuple/tuple_cat "std::tuple_cat"
 [previous]: /cxx11/2012/12/09/optimal-tuple-iii.html "Previously..."
 [next]: /cxx11/2012/12/23/optimal-tuple-v.html "To be continued..."


