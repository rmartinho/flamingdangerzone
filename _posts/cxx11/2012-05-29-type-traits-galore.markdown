---
layout: post
title: More type traits
categories: cxx11
short: where I showcase cool traits
last_edit: 24 December 2012
---

The C++11 standard library provides us with several type traits, many of which
need compiler magic and thus cannot be reproduced within the language. These
type traits are very useful, but, at least for me, they still don't cover a lot
of common uses. This article will cover some useful traits that are not found in
the standard library.

### Identity metafunction

The first one is the simplest one. An identity metafunction is one that returns
its argument unchanged. It's the most basic type transformation: it performs no
transformation at all. The implementation is rather simple as well.

{% highlight cpp %}
template <typename T>
struct identity { using type = T; };
{% endhighlight %}

This metafunction already [exists in the Boost.MPL library][mpl-identity], but
since it is so simple, I don't mind writing it myself.

But, what is a "do-nothing" construct useful for?

Well, it is useful when we need to do something, but want to do nothing! An
example is when we use `std::conditional` and the branches are evaluated lazily.
If one of the branches is a known type instead of a type computation, we need to
make a no-op type computation out of it.

{% highlight cpp %}
template <typename T>
struct foo {
    using type = std::conditional<
        std::is_something<T>::value,
        identity<T>, // produce a T lazily
        foo<typename change<T>::type>
    >::type::type;
}
{% endhighlight %}

### Identity alias

We can define an identity alias that "invokes" `identity` by accessing its
`type` member.

{% highlight cpp %}
template <typename T>
using Identity = typename identity<T>::type;
{% endhighlight %}

Is this one as useful as the other one?  Why not simply write the type directly
instead of wrapping it in `Identity`?

There is one useful property of this alias: since dependent names cannot take
part in type deduction, we can use this alias to prevent type deduction. This
isn't very common, but it does pop up sometimes. In the standard library we have
`std::forward` as an example of a function where type deduction is not desired.
Since this is the only meaningful use case for this alias, I prefer to give it a
more appropriate name:

{% highlight cpp %}
template <typename T>
void no_deduction(NotDeducible<T> x);
{% endhighlight %}

And what if we wrote this alias directly without invoking `identity`?

{% highlight cpp %}
template <typename T>
using Alias = T;
{% endhighlight %}

This is truly mostly useless. I only know two use cases for it, and they're both
shady.

We can use it to [create a temporary array][temporary-array], but who would want
that?

We can also use it to work around the quirks of the C declarator syntax:

{% highlight cpp %}
void f(alias<int[10]>& a);
// instead of void f(int(&a)[10]>);

alias<int[10]>& f();
// instead of int (&f())[10]
{% endhighlight %}

And this is already too much talking about constructs that do nothing.

### Dependent boolean

`static_assert` is a new language feature that allows us to produce errors when
a certain compile-time boolean expression is false. One use case of this feature
is providing custom error messages when using templates. Take for example,
`std::result_of`. Could it be implemented as follows?

{% highlight cpp %}
template <typename T>
struct result_of {
    static_assert(false, "T must be a signature");
};
template <typename F, typename... A>
struct result<F(A...)> {
    // actual implementation goes here
};
{% endhighlight %}

It could not. The problem with this is that the static assert will *always*
trigger, even if the primary template is not instantiated. To prevent it from
triggering we need to make it use a dependent name. I use something like
`std::false_type`, but with a type parameter. Or better, a variadic parameter
pack, for when we're writing variadic templates.

{% highlight cpp %}
template <bool B, typename...>
struct dependent_bool_type : std::integral_constant<bool, B> {};
// and an alias, just for kicks :)
template <bool B, typename... T>
using Bool = typename dependent_bool_type<B, T...>::type;
{% endhighlight %}

With it the primary template can be made to work now.

{% highlight cpp %}
template <typename T>
struct result_of {
    static_assert(Bool<false, T>::value, "T must be a signature");
};
{% endhighlight %}

This can also be used for writing SFINAE-based traits, but I'm leaving that for another
article.

### Conditions and logical meta-ops

Several of the standard library traits provide a boolean `value` member. And
some like `std::conditional` and `std::enable_if` take a boolean as a parameter.
`std::enable_if` is a special case, so I will leave it for a future article.

One could write a simple `Conditional` alias as follows.

{% highlight cpp %}
template <bool If, typename Then, typename Else>
using Conditional = typename std::conditional<If, Then, Else>::type;
{% endhighlight %}

More often than not, the boolean parameter will be the `value` member of some
type trait. In order to reduce boilerplate, I prefer to have `Conditional`
simply take a type trait and automatically access its `value`.

{% highlight cpp %}
template <typename If, typename Then, typename Else>
using Conditional = typename std::conditional<If::value, Then, Else>::type;

// usage becomes
Conditional<std::is_const<T>, A, B>
{% endhighlight %}

This is dandy if the condition is a simple type trait. But when the condition
is, for example, a conjunction of two traits, it does not work very well. Using
the first definition of `Conditional`, code would look like this:

{% highlight cpp %}
Conditional<
    std::is_const<RemovePointer<T>>::value && std::is_pointer<T>::value,
    A, B>
{% endhighlight %}

With the second definition, we can't write this. Unless we make metafunctions
that work on boolean traits just like the logical operators do for boolean
values!

Making the *and* and *or* meta-operations binary would again make it hard to
read when there are more than two conditions involved, so let us make them
variadic templates.

{% highlight cpp %}
// Meta-logical negation
template <typename T>
using Not = Bool<!T::value>;

// Meta-logical disjunction
template <typename... T>
struct any : Bool<false> {};
template <typename Head, typename... Tail>
struct any<Head, Tail...> : Conditional<Head, Bool<true>, any<Tail...>> {};

// Meta-logical conjunction
template <typename... T>
struct all : Bool<true> {};
template <typename Head, typename... Tail>
struct all<Head, Tail...> : Conditional<Head, all<Tail...>, Bool<false>> {};

// And usage looks like this:
Conditional<
    all< std::is_const<RemovePointer<T>>, std::is_pointer<T> >
    A, B>
{% endhighlight %}

### Bare types

Sometimes I need to strip a type of any references and cv-qualifiers, usually to
compare it to a known non-reference, non-cv type. I call these *bare types*.

Transforming a type into a bare type can be achieved with a combination of
`RemoveReference` and `RemoveCv`, but I prefer to have a specialized trait
anyway, especially since the two must be combined in a specific order:
`RemoveCv< RemoveReference< int const& > >` is `int`, while
`RemoveReference< RemoveCv< int const& > >` is `int const`. Having a specific
trait avoids this potential mistake.

{% highlight cpp %}
template <typename T>
using Bare = RemoveCv<RemoveReference<T>>;
{% endhighlight %}

### Reference and cv-qualifier propagators

The standard library has `std::is_const`, `std::remove_const`, `std::add_const`,
and similar traits for `volatile` and references, that test, add, or remove
those qualifications from a type. I find it lacks one other type of operation
with those qualifications: copying the qualifications from one type to another.
Implementing this for cv-qualifiers is quite trivial: use the testing trait, and
if it yields true, use the adding trait.

{% highlight cpp %}
// Produces Destination as const if Source is const
template <typename Source, typename Destination>
using WithConstOf = Conditional<std::is_const<Source>, AddConst<Destination>, Destination>;
// Produces Destination as volatile if Source is volatile
template <typename Source, typename Destination>
using WithVolatileOf = Conditional<std::is_volatile<Source>, AddVolatile<Destination>, Destination>;
// Produces Destination as const if Source is const, and volatile if Source is volatile
template <typename Source, typename Destination>
using WithCvOf = WithConstOf<Source, WithVolatileOf<Source, Destination>>;
{% endhighlight %}

Implementing them for the value category (i.e. object, lvalue reference, rvalue
reference) is a bit more involved, but nothing too complicated: two nested
conditionals.

{% highlight cpp %}
// Produces Destination with the same value category of Source
template <typename Source, typename Destination>
using WithValueCategoryOf = Conditional<std::is_lvalue_reference<Source>,
                                AddLvalueReference<Destination>,
                                Conditional<std::is_rvalue_reference<Source>,
                                    AddRvalueReference<Destination>,
                                    Destination>>;
{% endhighlight %}

 [mpl-identity]: http://www.boost.org/doc/libs/release/libs/mpl/doc/refmanual/identity.html "boost::mpl::identity"
 [temporary-array]: http://stackoverflow.com/a/10624677/46642

