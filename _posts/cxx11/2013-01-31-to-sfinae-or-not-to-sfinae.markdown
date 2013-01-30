---
layout: post
title: To SFINAE or not to SFINAE
categories: cxx11
short: what should the short be?
published: false
---

`enable_if` is somewhat of a hack used to exploit a language feature (SFINAE)
for selectively enabling or disabling certain overloads based on compile-time
tests. SFINAE can sometimes be cumbersome, ugly, and cryptic, and isn't known
for producing the clearest error messages. As I've shown before, some of these
annoyances can be mitigated with some clever changes to the typical patterns,
but it still feels like a hack.

There are some relatively well-known alternatives to SFINAE that seem to fulfill
the same purpose, and don't have many of the annoyances of SFINAE: like tag
dispatching and static assertions. But can these alternatives fully replace
things like `enable_if`?

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
right one. This may appears to do the same one can do with SFINAE, but it
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

As I alluded above, there are trade-offs to be made here. 

--- from now on, just copy-pasted stuff from comments to flesh out ---

Short? Sorry :( Imagine you implement a function f as a template for all types
in that fulfill some condition and not for other types. If you use tag
dispatching, you won't have a `false_type` overload. That's fine, it causes an
error when you try to use that f for the wrong types. You don't even need tag
dispatching for this, you could just use `static_assert` and give better error
messages. (to be continued) – R. Martinho Fernandes 7 mins ago 
    
But now consider a trait `is_f_able` that tells you if the expression
`f(std::declval<T>())` is valid. That trait will likely use some form decltype on
that expression to detect this (the usual "has member" kind of trait). Problem
is, `decltype(f(std::declval<T>()))` will either "work" (i.e. not fail
substitution) or produce a static assertion failure. Neither of these options is
desirable because it means you never get a `::value` false from that trait. SFINAE
makes the function not be there, and the trait detects that correctly to produce
a false.

