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
    "this test is only meaningful if the standard tuple can be an empty type");
    // both libc++'s and libstdc++'s tuples are empty types when possible
static_assert(not std::is_empty<tuple<>>::value,
    "our tuple is not an empty type when it should");
{% endhighlight %}

This is a minor issue, but if we want to have a drop-in replacement, we want to
preserve as much as the original behaviour as possible.

We can preserve emptiness inheriting from the optimal storage instead. Since we
don't want to expose the storage, nor we want implicit conversions to mess
things up, we are going to inherit privately.

{% highlight cpp %}
template <typename... T>
struct tuple : private OptimalStorage<T...> {
    // interface goes here
};
{% endhighlight %}

 [previous]: /cxx11/2012/12/09/optimal-tuple-iii.html "Previously..."
 [next]: /cxx11/2012/12/23/optimal-tuple-v.html "To be continued..."


