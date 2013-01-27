---
layout: post
title: Size matters, part 4
categories: cxx11
short: where I finally implement the tuple class
---

Now that we can [map all indices back and forth][previous] between the interface
and the storage we can start fleshing out the interface of our tuple. A worthy
goal is to provide exactly the same interface as the standard tuple class, so
that we can use ours as a drop-in replacement.

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
implementation. If we take a look at it we can see that most members are
actually [constructors]. There's very little to do with a tuple other than
constructing and accessing elements.  Some important parts of the interface are
provided as non-member functions ([`get`][get], three [factories], [relational
operators], and [`tuple_cat`][tuple_cat]). And then we have the type traits like
[`std::tuple_element`][tuple_element] that we will want to specialize in the
`std` namespace.

Let's go through all of the members and non-members to get a general overview of
their implementations.

- The default constructor is dead simple: default constructing the storage works
fine, so the compiler generated one is ok;
- the two variadic constructors can be delegated to the `std::tuple`
constructors by mapping from storage to interface;
- the copy and move constructors, as well as the copy and move assignment
operators can be compiler generated, since the underlying tuple already does all
the work;
- the constructors or assignment operators that convert from tuples with
different elements can delegate to the `std::tuple` constructor using our map
from storage to interface indices;
- the uses-allocator constructors are similar to their respective regular
constructors;
- the factories can be simply implemented by delegating to the constructor of
the right tuple types; our tuple does not give a big advantage for `tie` and
`forward_as_tuple`, but implementing them is cheap anyway;
- `tuple_cat` is probably the most complicated of all; it will require some more
complicated mappings and will be the subject of a later post;
- `get` is to use our interface to storage mappings;
- everything else can be simply delegated to the underlying standard tuple.
- our tuple will also want to have constructors that accept standard tuples
directly; those can be implemented in a fashion similar to the constructors that
take tuples of different elements.

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
and `const&`. These will need access to the underlying tuple, so we will declare
it a friend of our tuple.

When we have a call like `get<I>(t)`, we need to map `I` to a storage index and
then just call the `std::get` on that index. For that we can use `tuple_element`
on the map we computed since our maps are actually tuples
[<span id="ref1">&dagger;</span>][ftn1]. That leaves us with the following
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

[<span id="ftn1">&dagger;</span>][ref1] There is currently a bug in GCC 4.7.2 that causes
trouble for carrying a pack of indices as a `std::tuple`, so it may be necessary
to use a few workarounds. One alternative involves using a custom type to pack
the indices and a specialization of `std::tuple_element` for that type.

 [ftn1]: #ftn1
 [ref1]: #ref1

### Shuffle and forward

As should be obvious by now, we will need some way to forward a pack of
arguments appropriately shuffled according to one of our maps.

Recall that our maps are just packs of indices. With a pack of indices as a
variadic template parameter pack we can simply expand to `std::get<I>` or
similar and we have all the elements in the desired order. For that to work we
also need to have the arguments as a tuple. We will assume it is a tuple of
references because we want perfect forwarding all over (i.e. it will be the
result of `std::forward_as_tuple`).

This function to shuffle and forward a tuple will need to return the shuffled
tuple. It is rather easy to make an alias to compute that shuffled tuple for the
return type. Then we just need to forward the element at each index given in the
map into a new tuple of references.

{% highlight cpp %}
template <typename Tuple, std::size_t... I>
using ShuffleTuple = std::tuple<TupleElement<I, Tuple>...>;

template <std::size_t... I, typename Tuple>
ShuffleTuple<Tuple, I...> forward_shuffled_tuple(indices<I...>, Tuple&& t) {
    return std::forward_as_tuple(std::get<I>(std::forward<Tuple>(t))...);
}
{% endhighlight %}

Since we will need this for the tuple constructors as well, let us write a
version that takes the arguments as a variadic pack, instead of a tuple. All it
needs to do is to forward them as a tuple into the function above.

{% highlight cpp %}
template <std::size_t... I, typename... T>
ShuffleTuple<std::tuple<T&&...>, I...> forward_shuffled(indices<I...> map, T&&... t) {
    return forward_shuffled_tuple(map, std::forward_as_tuple(std::forward<T>(t)...));
}
{% endhighlight %}

There is one important thing to note in the return type. We cannot use
`std::tuple<T...>`, because when, for example, we pass in an int rvalue, `T` is
deduced as `int`, not as `int&&` (and `T&&` becomes `int&&`). So we use
`std::tuple<T&&...>` to make sure we have a tuple of references and the
reference collapsing rules make sure the references are of the right kind.

Now it's easy to implement all those constructors: all that is needed is to pass
along the map.

{% highlight cpp %}
explicit tuple(T const&... t)
: storage_type { forward_shuffled(to_interface{}, t...) } {
    // static assertion to get decent errors
    static_assert(All<std::is_copy_constructible<T>...>::value,
        "all tuple element types must be copy constructible");
}
template <typename... U,
          EnableIf<std::is_convertible<U, T>...>...>
explicit tuple(U&&... u)
: storage_type { forward_shuffled(to_interface{}, std::forward<U>(u)...) } {
    static_assert(sizeof...(T) == sizeof...(U),
        "number of constructor parameters must match tuple size");
}
{% endhighlight %}

The constructors that take tuples of different elements will need appropriate
our helper functions to use our `get` function instead of `std::get`. ADL can
help us here.

{% highlight cpp %}
template <std::size_t... I, typename Tuple>
ShuffleTuple<Tuple, I...> forward_shuffled_tuple(indices<I...>, Tuple&& t) {
    using std::get; // bring std::get into scope
    return std::forward_as_tuple(get<I>(std::forward<Tuple>(t))...);
}
{% endhighlight %}

### Dealing with reference wrappers

We are almost done with the non-boring work. I will just make a short stop to
talk about [`make_tuple`][make_tuple]. As can be read in the documentation,
`make_tuple` has some special treatment for `reference_wrapper`.

Without `reference_wrapper` we would never be able to make a tuple where some
elements are references using `make_tuple`. `make_tuple` is important because it
saves us from writing out tuple types and lets them be inferred. By default it
always deduces non-reference types, but it lets us use  `reference_wrapper` to
have it deduce an actual reference.

{% highlight cpp %}
int n = 1;
auto t = std::make_tuple(std::ref(n), n);
static_assert(std::is_same<decltype(t), std::tuple<int&, int>,
    "make_tuple unwraps reference_wrappers");
{% endhighlight %}

To get this for our version of `make_tuple` we need to write a trait that turns
`reference_wrapper<T>` into `T&` and that decays all other types into value
types.

{% highlight cpp %}
template <typename T>
struct unwrap_reference
: identity<T> {};

template <typename T>
struct unwrap_reference<std::reference_wrapper<T>>
: identity<T&> {};

template <typename T>
struct decay_reference : unwrap_reference<Decay<T>> {};
template <typename T>
using DecayReference = typename decay_reference<T>::type;
{% endhighlight %}

With this writing `make_tuple` is now very simple. Reference wrappers convert
implicitly to references, so we can simply forward them directly; only the
return type needs to be right.

{% highlight cpp %}
template <typename... T>
tuple<DecayReference<T>...> make_tuple(T&&... t) {
    return tuple<DecayReference<T>...>(std::forward<T>(t)...);
}
{% endhighlight %}

 [make_tuple]: http://en.cppreference.com/w/cpp/utility/tuple/make_tuple "std::make_tuple reference"

### That's not all, folks!

Most of the rest of the implementation is either trivial, or extremely similar
to what we implemented already. Some workarounds may be needed to work around
bugs in the implementations of variadic templates.

The only function worth of mention now is `tuple_cat`. This one is quite tricky,
so I will leave it for [the next installment].

 [implementation]: http://github.com/rmartinho/rmartinho.github.com/tree/master/code/tuple "Code on GitHub"
 [previous]: /cxx11/2012/12/09/optimal-tuple-iii.html "Previously..."
 [next]: /cxx11/2012/12/23/optimal-tuple-v.html "To be continued..."

