---
layout: post
title: Remastered enable_if
categories: cxx11
short: where I fix enable_if
last_edit: 02 Jun 2012
---

Without the language support for concepts, one cannot directly overload template
functions based solely on the properties of type parameters.

{% highlight cpp %}
template <Scalable T> // pretend concept syntax
T twice(T t) { return 2*t; }

template <typename T>        // assuming non-concept templates are
T twice(T t) { return t+t; } // picked after the ones with concepts
{% endhighlight %}

Without concepts, all template type parameters are of the same kind
(`typename`), and that makes the following definitions define the same template:

{% highlight cpp %}
template <typename Scalable> // name is only descriptive
Scalable twice(Scalable t) { return 2*t; }

template <typename Additive> // name is only descriptive
Additive twice(Additive t) { return t+t; }
{% endhighlight %}

To achieve similar behaviour in the current specification of the language, one
often takes advantage of particular overload resolutions rules.

### Interlude: A brief SFINAE rehash

The idea is very simple: if substituting a template parameter
during overload resolution produces invalid code, the compiler simply ignores
the offending overload instead of generating an error. This is commonly know as
"*substitution failure is not an error*", usually abbreviated to SFINAE.

Consider the following example:

{% highlight cpp %}
template <typename Pointer>
typename std::pointer_traits<Pointer>::element_type foo(Pointer const& p);

template <typename Iterator>
typename std::iterator_traits<Iterator>::value_type foo(Iterator const& it);

std::unique_ptr<int> u;
std::vector<int> v;
int* p;

foo(u);         // (1)
foo(v.begin()); // (2)
foo(p);         // (3)
{% endhighlight %}

When the compiler tries to resolve the overload for these functions calls, it
will find two `foo`s, deduce the template parameters for them, and then
substitute them. For function call (1), that means instantiating
`std::pointer_traits<std::unique_ptr<int>>::element_type` and
`std::iterator_traits<std::unique_ptr<int>>::value_type`. The first one results
in the type `int`, but the second one fails, because `std::unique_ptr<T>`
doesn't have a specialization of `std::iterator_traits`. The compiler will
discard the second overload and pick the only valid one, the first.

A similar thing happens in call (2), except in this case, it's the first
overload that gets discarded in a similar fashion.

In (3), the substitution process will suceed in both overloads, and the result
will be ambiguous because there is no preference between any of the two
overloads.

This example shows how one can take advantage of SFINAE to select different
templated overloads of a function based on a property of the template
parameters. In this case, the property was merely the existence or not of
specializations of `std::iterator_traits` or `std::pointer_traits`. Often one
wants to do the selection based on a boolean value, though.

### The archaic `std::enable_if`

The standard library provides a `std::enable_if` metafunction that can be used
to exploit SFINAE to select overloads based on a boolean condition.

{% highlight cpp %}
template <bool Condition, typename T = void>
struct enable_if {}; // no members

template <typename T>
struct enable_if<true, T> { using type = T; }; // type member
{% endhighlight %}

This metafunction is used by placing an access to its `type` member somewhere
in a function template declaration. When the condition is false, substitution
fails (because there's no `type`) and the overload is ignored. When the
condition is true, it is substituted by the second template parameter.

But where can we place it in a function template declaration?

In C++03, it was often placed in the return type of the function, or as an extra
defaulted parameter. None if these is an universal option: the first one is not
usable in constructors, because they don't have a return type; and the second
one is not usable in most operator overloads, because the number of arguments is
fixed.

We could thus have something like the following:

{% highlight cpp %}
template <typename T>
typename std::enable_if<is_scalable<T>::value, T>::type
twice(T t) { return 2*t; }

template <typename T>
T twice(T t, typename std::enable_if<!is_scalable<T>::value>::type* = 0) { return t+t; }
{% endhighlight %}

We need the two overloads to have disjoint conditions because to avoid
ambiguities, only one can be viable.

This style looks very arcane and makes it hard to read the return type in the
middle of all that. The extra parameter approach isn't really much better in
terms of readability.

### Evolution

As seen in [a previous article][dependent-names-bliss], we can make an alias
template to get rid of the `typename` and `::type` cruft. Just like in
`Conditional`, I'm changing the boolean parameter to a boolean trait type.

And since we often need a condition on one overload and its negation on another,
I'm making a `DisableIf` alias as well.

{% highlight cpp %}
template <typename Condition, typename T = void>
using EnableIf = typename std::enable_if<Condition::value, T>::type;

template <typename Condition, typename T = void>
using DisableIf = typename std::enable_if<!Condition::value, T>::type;
{% endhighlight %}

This certainly helps in the readability department, but it's still not
universally usable in the same way because we're still using it on return types
or function parameters.

Luckily, there is now a new place where one can place these `EnableIf`
constructs to select overloads. Because function templates can now have default
parameters, we can use an extra defaulted template parameter, and leave both the
return type and the function parameters free of any cruft.

{% highlight cpp %}
template <typename T,
          typename = EnableIf<is_scalable<T>>>
T twice(T t) { return 2*t; }

template <typename T,
          typename = DisableIf<is_scalable<T>>>
T twice(T t) { return t+t; }
{% endhighlight %}

Notice how the second parameter of `EnableIf` is no longer relevant here.
Previously, we were using it to provide the type that would be in the place we
were hijacking for `std::enable_if`. But now that is no longer necessary.

Unfortunately, this code doesn't compile anymore. The problem is that
substitution only happens upon overload resolution, but the compiler must parse
and do non-dependent lookups on the declarations once it sees them. And when it
does so, it will think the two declarations are declarations of the same
template. To better understand this issue, consider a similar thing with
defaulted function parameters:

{% highlight cpp %}
int foo(int x, int y = 17); // { return x+y; }
int foo(int x, int y = 23); // { return x*y; }
{% endhighlight %}

This code is ill-formed: it declares the same function twice, but with different
default parameters. The same problem, but at a different level, happens with the
templates above: the same function template is declared twice, with different
default template parameters. The compiler will reject it even if there are no
calls to the functions.

To make the two templates be different, we can do one of two things: change
their number, or change their kind.

#### Changing the number

Changing their number is simple: we just add an extra dummy defaulted parameter
to one of them.

If we did so with the `foo` functions above, we would still be in
trouble, because, while the compiler would accept the two declarations as
different functions, we wouldn't be able to call them without ambiguity. But in
the case of the templates, we are trying to get one of them to be dropped
because of substitution failure, so that won't be a problem.

{% highlight cpp %}
template <typename T,
          typename = EnableIf<is_scalable<T>>>
T twice(T t) { return 2*t; }

template <typename T,
          typename = DisableIf<is_scalable<T>>,
          typename /*Dummy*/ = void>
T twice(T t) { return t+t; }
{% endhighlight %}

This technique works fine, but it may not be very practical if there are more
than two overloads to select from: each one needs one more dummy than the
previous one.

#### Changing the kind

Changing the kind of the parameter where the substitution failure happens
means it won't be a type parameter anymore. The simplest alternative, is to make
it a non-type parameter. But of what type should that be? `int`? `bool`?
Something else entirely?

We can't make it something like `int = ...` because that will bring us back to
the same problems: we need a different type for each argument.

There is one thing that is different about the two templates that we can use
here: the `EnableIf` expressions themselves. Instead of a type parameter, we can
use a non-type parameter whose type is given by `EnableIf` directly.

{% highlight cpp %}
template <typename T,
          EnableIf<is_scalable<T>>>
T twice(T t) { return 2*t; }

template <typename T,
          DisableIf<is_scalable<T>>>
T twice(T t) { return t+t; }
{% endhighlight %}

For this to actually work, we need to solve a couple problems first. We need to
pick a type for the second parameter of `EnableIf`, and we need to make it so
that the calling code doesn't need to pass something for that parameter.

There are only a handful of types we can use there: integral types, pointers,
references, or enumerations. The default `void` isn't one of them.

The type we pick needs to be as inocuous as possible. Ideally, it would make it
impossible for the user to pass anything for that parameter explicitly, like
`twice<int, 17>(23)`. But the best we can achieve is to prevent the user from
doing so accidentally (and if they do so on purpose, they get what they
deserve), so let's stick with that.

Of all the types we can pick, a scoped enumeration without any enumerators is
probably the best: passing an instance of it requires using its name explicitly,
and if we hide the name in the implementation you can't really do it by
accident.

{% highlight cpp %}
namespace detail {
    enum class enabler {};
}
{% endhighlight %}

We have our type, but how do we make it so the user doesn't have to provide a
value for the extra parameter? There are two options. We can use a default
argument; or we can make it a variadic parameter pack, which will be deduced as
empty.

If we use a default argument, we need a value. A variadic pack doesn't need one,
so it is a better option. That leaves us with the following implementation and
usage:

{% highlight cpp %}
template <typename Condition>
using EnableIf = typename std::enable_if<Condition::value, detail::enabler>::type;

template <typename T,
          EnableIf<is_scalable<T>>...>
T twice(T t) { return 2*t; }
{% endhighlight %}

Implementation of `DisableIf` is left as an exercise for the reader.

In my opinion this is a big improvement over the old style: it's a universally
applicable style; the condition used to select the overload is out of the actual
function signature; and it has no cruft, except for the three dots at the end.

The big problem with this is that compilers are not yet perfect in their C++11
support. My tests show that GCC 4.7 is up for the task, but Clang 3.1 isn't yet.
For clang I use the following workaround.

{% highlight cpp %}
constexpr detail::enabler dummy = {};

template <typename Condition>
using EnableIf = typename std::enable_if<Condition::value, detail::enabler>::type;

template <typename T,
          EnableIf<is_scalable<T>> = dummy>
T twice(T t) { return 2*t; }
{% endhighlight %}

### And an extra tweak

Since sometimes more than one condition is needed, I like to use a slightly
tweaked version.

{% highlight cpp %}
template <typename... Condition>
using EnableIf = typename std::enable_if<all<Condition...>, detail::enabler>::type;

template <typename T,
          EnableIf<is_scalable<T>, is_something_else<T>>...>
T twice(T t) { return 2*t; }
{% endhighlight %}

 [dependent-names-bliss]: /2012/05/27/dependent-names-bliss.html

