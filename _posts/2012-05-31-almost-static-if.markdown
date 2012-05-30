---
layout: post
title: Remastered enable_if
---

Without the language support for concepts, one cannot directly overload template
functions based solely on the properties of type parameters.

{% highlight cpp %}
template <MultipliableByScalar T> // pretend concept syntax
T twice(T t) { return 2*t; }

template <Additive T> // pretend concept syntax
T twice(T t) { return t+t; }
{% endhighlight %}

Without concepts, all template type parameters are of the same kind
(`typename`), and that makes the following overloads completely ambiguous:

{% highlight cpp %}
template <typename MultipliableByScalar> // name is only descriptive
MultipliableByScalar twice(MultipliableByScalar t) { return 2*t; }

template <typename Additive> // name is only descriptive
Additive twice(Additive t) { return t+t; }
{% endhighlight %}

To achieve similar behaviour in the current specification of the language, one
often takes advantage of some particular overload resolutions rules.

### Interlude: A brief SFINAE rehash

The idea behind SFINAE is very simple: if substituing a template parameter
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
