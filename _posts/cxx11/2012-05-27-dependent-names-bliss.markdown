---
layout: post
title: Handling dependent names
categories: cxx11
---

### Annoying `typename`

Let's consider the example of converting an enumerator to the corresponding
value of the enumeration underlying type. In the old C++ we would always have
such a conversion made implicitly, but now there is another choice. *Scoped
enumerations*, informally known as `enum class`es, do not have such an implicit
conversion. The conversion is still possible, but it must be done explicitly.

{% highlight cpp %}
enum class foo : int { bar = 42 };

int main() {
    int underlying = static_cast<int>(foo::bar);
    assert(underlying == 42);
}
{% endhighlight %}

Doing this requires us to know what the underlying type is. But if we are
writing generic code that works with various enums, we do not have that
knowledge. The standard library header `<type_traits>` provides us with the type
transformation `std::underlying_type` for this purpose. With it one can write a
generic function that works for any enum and does not require knowing the
underlying type.

{% highlight cpp %}
#include <type_traits>
#include <cassert>

template <typename Enum>
std::underlying_type<Enum>::type to_underlying(Enum enum) {
    return static_cast<std::underlying_type<Enum>::type>(enum);
}

enum class foo : int { bar = 42 };

int main() {
    int underlying = to_underlying(foo::bar);
    assert(underlying == 42);
}
{% endhighlight %}

But if we try to compile this... it does not compile. Ooops. Clang produces the
following error:

    foo.cpp:3:1: fatal error: missing 'typename' prior to dependent type name 'std::underlying_type<Enum>::type'
    std::underlying_type<Enum>::type to_underlying(Enum enum) {
    ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    typename 
    1 error generated.

The code is missing [the `typename` disambiguator][typename]. C++ needs that so
the compiler knows that the *dependent names* we used are type names.

{% highlight cpp %}
template <typename Enum>
typename std::underlying_type<Enum>::type to_underlying(Enum enum) {
    return static_cast<typename std::underlying_type<Enum>::type>(enum);
}
{% endhighlight %}

With this the function compiles, and works fine. In this example, the need for
`typename` is just a minor nuissance. But in a more complex scenario, it can be
quite annoying. Imagine if the `Enum` type parameter was `foo&` instead. Perhaps
it was not deduced from a parameter, but obtained as the result of some
`qux_trait` type transformation. Before `underlying_type` could be applied to
it, it would be necessary to remove the reference from it. The entire type
computation would look something like this:

{% highlight cpp %}
typename std::underlying_type<
    typename std::remove_reference<
        typename qux_trait<T>::type
    >::type
>::type
{% endhighlight %}

On one line that is very hard to read. All those `typename` keywords and
`::type` accesses add a lot of clutter. Until C++11, this was something one had
to put up with.

### Lovely alias templates

C++11 brings us *alias templates*, sometimes referred to as typedef templates.
These are basically typedefs that can have template parameters. The syntax for
declaring an alias template looks like this:

{% highlight cpp %}
template <typename T>
using type_erased_unique_ptr = std::unique_ptr<T, std::function<void(T*)>>;
{% endhighlight %}

I noticed two interesting properties of alias templates: by itself, an usage of
an alias template is not a dependent name; and an alias template can refer to a
dependent name. By combining these two, we have an alternative solution to the
problem that the `typename` disambiguator solves. As an example, consider the
following alias template:

{% highlight cpp %}
template <typename T>
using better_underlying_type = typename std::underlying_type<T>::type;
{% endhighlight %}

With this, we can simply write `better_underlying_type<Enum>`, and since it is
not a dependent name, we don't need to prefix it with `typename`. By applying
the same idea to other traits, the triply nested example from above can be
rewritten as follows:

{% highlight cpp %}
better_underlying_type< better_remove_reference< better_qux_trait< T > > >
{% endhighlight %}

That looks almost readable now. We just need to get better names for our "better
traits". Ideally, we would like to keep the same names. Those from the standard
library we will have to keep in our own namespace anyway, so they won't conflict
with the ones from `std`. But what about our own traits like `qux_trait`?

One could move the original `qux_trait` to a `detail` namespace and name the
alias `qux_trait`. But in fact, it is not that good an option because these new
"better" traits do no completely replace the old style ones.

There are cases when one doesn't actually want to access the `type` member.
Perhaps doing so would trigger infinite recursion as in the following example:

{% highlight cpp %}
template <typename T>
struct foo {
    using type = std::conditional<
        std::is_something<T>::value,
        something<T>,
        // if we accessed ::type here,
        // it would result in infinite recursion
        foo<typename change<T>::type>
    >::type::type; // so we only access ::type
                   // on the result of std::conditional
}
{% endhighlight %}

In such situations one needs the "lazy" metafunction that doesn't actually get
"invoked" until we access `::type` on it. So we actually need to leave the
traditional metafunction around.

How to name the two styles of traits is obviously a subjective style matter. One
can add a `_lazy` prefix to the old ones, or put them in a `lazy` namespace. Or
one can use a casing convention for ones and another for the others.

Me, I settled upon the convention that `PascalCase` traits are aliases that
stand for the result of the corresponding metafunction, while `snake_case`
traits are the actual metafunctions that need `::type` to be "invoked". That's
the style I will be using in this series. With my style, our nested metafunction
example looks like this:

{% highlight cpp %}
UnderlyingType< RemoveReference< QuxTrait< T > > >
{% endhighlight %}

 [typename]: http://stackoverflow.com/a/613132/46642

