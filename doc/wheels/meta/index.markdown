---
layout: doc-wheels
title: Meta-programming tools
---

The header file `<wheels/meta.h++>` provides various meta-programming
facilities.

Many of these are just convenience shortcuts for the metafunctions provided in
the standard C++ `<type_traits>` header, as described in [*Handling dependent
names*][alias post], but some have slightly different behaviour. There is a
number of additional traits, some of them described previously in [*More type
traits*][traits post] and [*Remastered enable_if*][sfinae post].

### Identity-like traits

#### Invoke

> `Invoke<T>` is an alias for `typename T::type`.

This alias is used to define new shortcut aliases or for simply invoking a
metafunction on the fly without the `typename` disambiguator.

#### identity

> `identity<T>::type` is `T`.

This identity metafunction is used to define new traits or for lazily evaluated
type computations.

#### Alias

> `Alias<T>` is an alias for `T`.

This alias is useful to declare references to arrays and functions pointers
without dealing with the C declarator syntax.

#### NotDeducible

> `NotDeducible<T>` is an alias for `T` that prevents type deduction.

This alias is used to declare function arguments that cannot be deduced and
must be specified explicitly.

### Boolean-related traits

#### Bool

> `Bool<B, T...>` is an alias for `std::integral_constant<bool, B>` that depends
> on all types `T`.

This alias is used to make static assertions that always fail, but only fail
when the containing template is instantiated. It also doubles as a quick boolean
meta-constant. It does not take part in type deduction.

#### Conditional

> `Conditional<If, Then, Else>` is an alias for `Then` if `If::value` is `true`,
> or an alias for `Else` if `If::value` is `false`.

This type trait is an improved variant of `std::conditional`.

#### Not

> `Not<T>` is a boolean meta-constant with the value `!T::value`.

#### Any

> `Any<T...>` is a boolean meta-constant with the value `true` if any of
> `T::value` is `true`, or `false` otherwise. It is evaluated in a
> short-circuiting manner.

#### All

> `All<T...>` is a boolean meta-constant with the value `true` if all of
> `T::value` are `true`, or `false` otherwise. It is evaluated in a
> short-circuiting manner.

### SFINAE traits

#### Void

> `Void<T>` is an alias for `void` that depends on `T`.

#### EnableIf

> `EnableIf<T...>` causes a substitution failure is any of `T::value` is
> `false`; it is an alias to a private enumeration type otherwise.

#### DisableIf

> `DisableIf<T...>` causes a substitution failure is all of `T::value` are
> `true`; it is an alias to a private enumeration type otherwise.

### Standard trait aliases

#### Boooooring

### Extensions to the standard traits

#### WithXOf

#### StorageFor

> `StorageFor<T...>` is an alias to a POD type with adequate size and alignment
> requirements for storing any type `T`.

### Other traits

#### DecayReference

#### Unqualified

> `Unqualified<T>` is an alias to `T` after removing reference and
> cv-qualifiers.

#### is\_unqualified

#### is\_callable

#### ResultOf

 [alias post]: /cxx11/2012/05/27/dependent-names-bliss.html
 [traits post]: /cxx11/2012/05/29/type-traits-galore.html
 [sfinae post]: /cxx11/2012/06/01/almost-static-if.html
