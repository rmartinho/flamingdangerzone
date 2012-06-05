---
layout: post
title: Optional return type deduction
---

Imagine a situation where one wants to write a function whose return type can be
computed from its parameter types (which can be deduced), but also wants to let
the caller explicitly pick a return type, overriding that computation.

As an example, consider the following factory function for `std::array`:

{% highlight cpp %}
template <typename... T,
          typename Array = std::array<Decay<CommonType<T...>>, sizeof...(T)>>
Array make_array(T&&... values) {
    return Array { { std::forward<T>(values)... } };
}

auto x = make_array(1, 2, 3);
auto y = make_array(1, 2, 3.0);
{% endhighlight %}

In that example, `x` will have type `std::array<int, 3>` and `y` will have type
`std::array<double, 3>`. But what if we want the first to be `std::array<double,
3>` and the second to be `std::array<double, 5>` (value-initializing the missing
elements)?

The goal is to have all of this compiling:

{% highlight cpp %}
auto x1 = make_array(1, 2, 3);
static_assert(std::is_same<std::array<int,3>, decltype(x1)>::value,
              "should compute the return type when not explicit");

auto x2 = make_array<double>(1, 2, 3);
static_assert(std::is_same<std::array<double,3>, decltype(x2)>::value,
              "should allow overriding the element type");

auto x3 = make_array<5>(1, 2, 3);
static_assert(std::is_same<std::array<int,5>, decltype(x3)>::value,
              "should allow overriding the size");

auto x4 = make_array<double, 5>(1, 2, 3);
static_assert(std::is_same<std::array<double,5>, decltype(x4)>::value,
              "should allow overriding both the element type and the size");
{% endhighlight %}

### Optional explicit type

The first assertion can already be satisfied. Let us look at the second one.

