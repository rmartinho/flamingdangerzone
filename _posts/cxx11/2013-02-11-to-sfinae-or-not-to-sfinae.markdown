---
layout: post
title: To SFINAE or not to SFINAE
categories: cxx11
short: where I wonder if SFINAE is really needed
---

`enable_if` is somewhat of a hack used to exploit a language feature (SFINAE)
for selectively enabling or disabling certain overloads based on compile-time
tests. SFINAE can sometimes be cumbersome, ugly, and cryptic, and isn't known
for producing the clearest error messages. As I've [shown before], some of these
annoyances can be mitigated with some clever changes to the typical patterns,
but it still feels like a hack.

There are some relatively well-known alternatives to SFINAE that seem to fulfill
the same purpose, and don't have many of the annoyances of SFINAE: like tag
dispatching and static assertions.

**Can these alternatives fully replace things like `enable_if`?**

 [shown before]: /cxx11/2012/06/01/almost-static-if.html 

### Tag dispatching

If you want to pick between two (or more) overloads, you can use the tag
dispatching technique. It consists of using overload resolution to pick one of
two (or more) implementations, where the overloads differ in the result of
applying some trait.

Example 1:

{% highlight cpp %}
#include <type_traits>

template <typename T>
int  f_impl(T t, std::true_type) {
    return 42 + t;
}
template <typename T>
double f_impl(T, std::false_type) {
    return 3.14;
}
template <typename T>
auto f(T t) -> decltype(f_impl(t, std::is_integral<T>())) {
    return f_impl(t, std::is_integral<T>());
}

#include <iostream>

int main() {
    std::cout << f(1) << " " << f(1.2);
}

// outputs "43 3.14"
{% endhighlight %}

`std::is_integral` derives from `std::true_type` or from `std::false_type`
depending on whether the type is integral or not. The `f_impl` overloads take
one of those types as argument and that's how overload resolution picks the
right one. This may appear to do the same one can do with SFINAE, but it
doesn't. It does something that is similar but at times not appropriate, as we
will see later.

### Static assertions

Sometimes we don't use SFINAE to pick between two overloads. Sometimes we use
SFINAE to just enable or disable a single function template, i.e., cause errors
unless the function template is called with appropriate arguments types. The
compiler will complain that overload resolution failed, and possibly list that
template as the failed candidate.

We can get this same behaviour with static assertions, and with better errors on
top.

Example 2:

{% highlight cpp %}
#include <type_traits>

template <typename T>
int f(T t) {
    static_assert(std::is_integral<T>(), "T must be integral");
    return 42;
}

int main() {
    f(1.2);
}

// main.cpp:5:5: error: static assertion failed: T must be integral
{% endhighlight %}

This appears to be a superior option, but once more, it is just similar in some
aspects. It fails to achieve one effect one achieves with SFINAE.

### What we are picking between

First, I need to present two concepts that need to be explicitly called out.
When discussing these issues, I and some other people have come to refer to them
as "hard errors" and "soft errors".

#### Hard errors

A hard error is like your run-of-the-mill compiler error: it stops compilation
dead in its tracks, because it makes the program ill-formed. It could be that
there is a call to a non-existing member function somewhere in the body, or
there is a failed static assertion.

Example 3:

{% highlight cpp %}
#include <type_traits>

template <typename T>
int f() {
    static_assert(std::is_integral<T>::value, "T must be integral");
    return 42;
}

int main() {
    f<double>();
}

// main.cpp:5:5: error: static assertion failed: T must be integral
{% endhighlight %}

Example 4:

{% highlight cpp %}
#include <type_traits>

template <typename T>
int f() {
    T t;
    t.f();
}

int main() {
    f<double>();
}
// main.cpp:6:5: error: request for member 'f' in 't', which is of non-class
// type 'double'
{% endhighlight %}

#### Soft errors

Soft errors are the familiar substitution failures. Soft errors cause overloads
to be discarded, but by themselves don't cause the program to be ill-formed. If
soft errors result in all overloads being discarded, the end result is a hard
error, an overload resolution failure.

Example 5:

{% highlight cpp %}
#include <type_traits>

struct foo { int x; };

struct bar { double y; };

template <typename T>
auto get_it(T t) -> decltype(t.x) {
    return t.x;
}
template <typename T>
auto get_it(T t) -> decltype(t.y) {
    return t.y;
}

int main() {
    get_it(foo{});
}
{% endhighlight %}

In the second overload, when instantiated with T = foo, `decltype(t.y)` has an
invalid expression: T has no `y` member. However, `decltype` is special and
turns this into a soft error. End result? There is only one overload to pick,
and the program compiles.

However, if we use a third type `struct qux {};` instead, the soft errors will
take away all the overloads and that causes a hard error: overload resolution
fails.

> main.cpp:19:17: error: no matching function for call to `get_it(qux)`

#### Which are which

Herein lies the difference between `enable_if` and the two alternatives presented
above: `enable_if` generates soft errors, while the alternatives generate hard
errors.

This is so because SFINAE takes the functions away when resolving overloads, as
if they did not exist. Tag dispatching and `static_assert`, however, do not remove
functions from the set of candidates. Overload resolution will still find and
even select those functions. When selected, if their body is not valid (like
when having failed static assertion), the program is ill-formed.

Hopefully this distinction is now clear enough. Let's talk about why it matters.

### Why you should care

As I alluded above, there are trade-offs to be made here. Tag dispatching and
static assertions may be more convenient than SFINAE, but they don't provide all
the same effects, namely they result in different kinds of errors in a way that
can affect semantics. It is now time to answer the question I posed at the
beginning.

Consider the following two functions in some library:

{% highlight cpp %}
template <typename T, typename U>
T make_impl(U&& u, std::true_type) {
    return T(std::forward<U>(u));
}
template <typename T, typename U>
T make_impl(U&&, std::false_type) {
    return T(); // line 7
}

// construct a T with the given argument if constructible,
// otherwise default-construct
template <typename T, typename U>
T make(U&& u) {
    return make_impl(std::forward<U>(u), std::is_constructible<T, U>());
}
{% endhighlight %}

When we call `make<foo>(bar)`, there are three different possibilities:

1. `foo` can be constructed from `bar`: the first overload of `make_impl` is
   picked and the code compiles;
2. `foo` cannot be constructed from `bar` and can be default-constructed: the
   second overload is picked and the code compiles;
3. `foo` cannot be constructed from `bar` and cannot be default-constructed: the
   second overload is picked we have a compiler error on line 7.

Now, let us suppose we write a class with a template constructor with two
overloads: one for random-access iterators, and one for other iterators. We
would be doing this because we can make a more efficient implementation using
the capabilities of random-access iterators. However, let's say that our code
cannot work without bidirectional iterators, so the other overload will use
functionality from those. There is no way to implement the constructor with
input or forward iterators.

Static assertions alone won't do here: they can never be used to select between
two overloads. We could statically assert that the iterator passed in is a
forward iterator or better, but that won't help with picking an overload.

The only viable options are tag dispatching and SFINAE, since both can be used
to pick between overloads. Let's look at both in turn.

#### Implementing with tag dispatching

{% highlight cpp %}
// a little helper
template <typename Iterator>
using IteratorCategory = typename std::iterator_traits<Iterator>::iterator_category;

// with tag dispatching
struct foo {
public:
    foo() {}

    template <typename Iterator>
    foo(Iterator it)
    : foo(it, IteratorCategory<Iterator>()) {}

private:
    template <typename Iterator>
    foo(Iterator it, std::bidirectional_iterator_tag) {
        // do it with a bidirectional iterator
    }
    template <typename Iterator>
    foo(Iterator it, std::random_access_iterator_tag) {
        // do it with a random-access iterator
        do_it_with_a_random_access_iterator(it);
    }
};
{% endhighlight %}

By now it should be obvious what happens when we pass different kinds of
iterators: 

1. passing a random access iterator will pick the second overload and use the
   version optimized for random-access;
2. passing a bidirectional iterator will pick the second overload and use the
   non-optimized version;
3. passing an input iterator will pick neither overload and will result in a hard
   error (overload resolution failure).

#### Implementing with SFINAE

What about the implementation with SFINAE?

{% highlight cpp %}
// a little helper
template <typename Iterator>
struct is_random_access_iterator
: std::is_base_of<std::random_access_iterator_tag, IteratorCategory<Iterator>> {};

// with SFINAE
struct foo {
public:
    foo() {}

    template <typename Iterator,
              DisableIf<is_random_access_iterator<Iterator>>...>
    foo(Iterator it) {
        // do it with a bidirectional iterator
    }

    template <typename Iterator,
              EnableIf<is_random_access_iterator<Iterator>>...>
    foo(Iterator it) {
        // do it with a random-access iterator
    }
};
{% endhighlight %}

This gives us the same behaviour as the version with tag dispatching when using
random-access or bidirectional iterators.

It also gives the same result, but for slightly different reasons when using an
input iterator: the first overload will be picked, but the body will fail to
compile since it probably makes use of `--it` somewhere.

#### Traits need some consideration

Now, how does this relate to the function `make` presented at the start? Both
examples share one important characteristic here: they both cause hard errors
*because* a constructor is picked and does not have a valid body (remember that
in the case of tag dispatching, the dispatching constructor is always picked).

This characteristic interact poorly with traits like `std::is_constructible`.
Those traits only look at the declarations (otherwise, how would they even begin
to work when the definition is in another translation unit?). Unfortunately this
means that for both examples presented `std::is_constructible<foo, some_input_iterator>::value`
is true.

Because of that, when we call `make<foo>(some_input_iterator())`, this will
attempt to construct a foo with an argument, instead of default constructing
one, and that ends with a compiler error.

#### Implementing with SFINAE, fixed

The example with SFINAE, however, can be made to work correctly. We just need to
make sure the constructors are removed from the overload candidate set when the
argument is an input iterator.

{% highlight cpp %}
// with SFINAE, fixed
struct foo {
public:
    foo() {}

    template <typename Iterator,
              EnableIf<Not<is_random_access_iterator<Iterator>>,
                       is_bidirectional_iterator<Iterator>>...>
    foo(Iterator it) {
        // do it with a bidirectional iterator
    }

    template <typename Iterator,
              EnableIf<is_random_access_iterator<Iterator>>...>
    foo(Iterator it) {
        // do it with a random-access iterator
    }
};
{% endhighlight %}

With this both constructors will be discard when overload resolution is
performed, and `std::is_constructible<foo, some_input_iterator>::value` will be
false, as expected, and `make<foo>(some_input_iterator())` will correctly
default construct `foo`.

#### Implementing with both

One could also combine the two: use tag dispatching to pick between the
two overloads, and use SFINAE to disable the distpatcher when the iterator does
not meet the requirements.

{% highlight cpp %}
// with both!
struct foo {
public:
    foo() {}

    // SFINAE on dispatcher
    template <typename Iterator,
              EnableIf<is_bidirectional_iterator<Iterator>>...>
    foo(Iterator it)
    : foo(it, IteratorCategory<Iterator>()) {}

private:
    // tags as usual
    template <typename Iterator>
    foo(Iterator it, std::bidirectional_iterator_tag) {
        // do it with a bidirectional iterator
    }
    template <typename Iterator>
    foo(Iterator it, std::random_access_iterator_tag) {
        // do it with a random-access iterator
        do_it_with_a_random_access_iterator(it);
    }
};
{% endhighlight %}

### Conclusion

While convenient, tag dispatching cannot quite replace SFINAE, because sometimes
it is actually important to get rid of overloads. If a templated overload cannot
work with some set of template parameters, it should be constrained
appropriately if one wants any traits that check for its existence to work
properly.
