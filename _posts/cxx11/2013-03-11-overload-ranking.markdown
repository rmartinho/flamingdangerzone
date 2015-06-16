---
layout: post
title: Beating overload resolution into submission
categories: cxx11
short: where Xeo brings order into the SFINAE chaos
author: Xeo
author_link: http://stackoverflow.com/u/500104
---

### Battle Intro

When dealing with overloaded templates, it's often desirable to restrict certain
overloads so that they don't accidentally get selected. [A previous blog
post][enableif] already explains how to do that, the disadvantage being that you
need to have *non-overlapping conditions* (or *disjoint conditions*). Let's take
a silly example:

{% highlight cpp %}
template<unsigned N, EnableIf<is_multiple_of<N, 3>>...>
void print_fizzbuzz(){ std::cout << "fizz\n"; }

template<unsigned N, EnableIf<is_multiple_of<N, 5>>...>
void print_fizzbuzz(){ std::cout << "buzz\n"; }

template<unsigned N, EnableIf<is_multiple_of<N, 15>>...>
void print_fizzbuzz(){ std::cout << "fizzbuzz\n"; }

template<unsigned N, DisableIf<Any<is_multiple_of<N, 3>, is_multiple_of<N, 5>>>...>
void print_fizzbuzz(){ std::cout << N << "\n"; }
{% endhighlight %}
    
A call to `print_fizzbuzz<15>()` will be ambiguous, since the first three
overloads are all viable and enabled &mdash; after all, 15 is a multiple of 3, 5
and 15 itself. So, how do we go about disambiguating these?

 [enableif]: /cxx11/almost-static-if "Remastered enable_if"

### Domestic Abuse

Why, by abusing a language feature of course! And one that is in quite a close
relationship with overloading, to boot: Implicit conversions. I could bore you
with a slew of standardese at this point, but I think I'll just summarize:
overloads with the least amount of implicit conversions on each argument, if
any, are preferred.

The fix is pretty well known &mdash; overloading on `int` and `long`, and
passing a literal `0` as the argument. This is often used when defining a
trait's test functions:

{% highlight cpp %}
template<class T>
struct some_trait{
private:
  template<class U /*, possibly some SFINAE test here */>
  static auto test(int)
      -> decltype(/* possibly some Expression SFINAE test here */, void());
  template<class>
  static char test(long);
  
public:
  static constexpr value = std::is_void<decltype(test<T>(0))>::value;
};
{% endhighlight %}

(There is an [in-depth example / explanation][explanation] on Stack Overflow.)

The literal `0` is of type `int`, as such the call `test<T>(0)` will prefer the
first overload, if it's viable (e.g. not disabled via SFINAE). If that one is
not viable, the second overload will provide a fallback because `int` is
implicitly convertible to `long`. Perfect.

 [explanation]: http://stackoverflow.com/a/9154394/500104 "Is it possible to write a C++ template to check for a function's existence?"

### Your Princess Is In Another Castle

... or not, since it only allows to disambiguate two overloads. It also can't be
used in a binary fashion, as overload resolution will look at each argument in
isolation to determine ambiguity. It is, however, a good starting point &mdash;
we can impose a total order over an unordered overload set with this. Let's see
how we can increase the number of overloads.

There's one type of parameter that, by definition, will be selected as the very
last choice for implicit conversions &mdash; the ellipsis (C-style variadic
functions). With this, we can now order a total of **three** overloads, enough
for our original problem:

{% highlight cpp %}
template<unsigned N, EnableIf<is_multiple_of<N, 15>>...>
void print_fizzbuzz(int){ std::cout << "fizzbuzz\n"; }

template<unsigned N, EnableIf<is_multiple_of<N, 3>>...>
void print_fizzbuzz(long){ std::cout << "fizz\n"; }

template<unsigned N, EnableIf<is_multiple_of<N, 5>>...>
void print_fizzbuzz(...){ std::cout << "buzz\n"; }

template<unsigned N, DisableIf<Any<is_multiple_of<N, 3>, is_multiple_of<N, 5>>>...>
void print_fizzbuzz(...){ std::cout << N << "\n"; }
{% endhighlight %}
    
We also add the ellipsis to the fourth overload, since it needs to take
*something*, otherwise it would be ruled out thanks to not having enough
parameters. So the original call would now be `print_fizzbuzz<15>(0)`, working
all fine and dandy. However, I think seeing the types directly in the overload
is more noise than anything, so we should fix that with a few using-aliases and
a type for the ellipsis:

{% highlight cpp %}
using choice_1 = int;
using choice_2 = long;

struct otherwise{ otherwise(...){} };

template<unsigned N, EnableIf<is_multiple_of<N, 15>>...>
void print_fizzbuzz(choice_1){ std::cout << "fizzbuzz\n"; }

template<unsigned N, EnableIf<is_multiple_of<N, 3>>...>
void print_fizzbuzz(choice_2){ std::cout << "fizz\n"; }

template<unsigned N, EnableIf<is_multiple_of<N, 5>>...>
void print_fizzbuzz(otherwise){ std::cout << "buzz\n"; }

template<unsigned N, DisableIf<Any<is_multiple_of<N, 3>, is_multiple_of<N, 5>>>...>
void print_fizzbuzz(otherwise){ std::cout << N << "\n"; }
{% endhighlight %}

`otherwise` just wraps the ellipsis behind another conversion, which allows us
to give it a nice name.

### For Great Justice

There's something that still bugs me, though &mdash; the need for `DisableIf` to
make the fourth overload, which is otherwise unrestricted, disjoint from the
first three. And if we can impose a total order with three overloads, we can
surely get it to four, right?

Turns out we can go even farther than that. To do that, we have to take a step
back and not only look at implicit conversions of unqualified types (like `int`
and `long`), but also at how qualifications play into this. C++ actually
provides us with a total ordering of references to cv-qualified types already,
with every "extra" cv-qualification making the overload less preferred. Last but
not least, we have any kind of rvalue reference being preferred to an lvalue
reference-to-const for rvalue arguments.

{% highlight cpp %}
using choice_1 = int&&;
using choice_2 = int const&&;
using choice_3 = int const volatile&&;
using choice_4 = int const&;
{% endhighlight %}

Yielding the following overload set.
    
{% highlight cpp %}
void f(choice_1){}
void f(choice_2){}
void f(choice_3){}
void f(choice_4){}
void f(otherwise);
{% endhighlight %}
    
The above overloads would all be unambiguous when called as `f(0)`, no matter
how many of them exist. That alone doesn't get us much farther than before, but
if we add `long` into the mix...

{% highlight cpp %}
using choice_5 = long&&;
using choice_6 = long const&&;
using choice_7 = long const volatile&&;
using choice_8 = long const&;
{% endhighlight %}
    
... and adding the respective overloads to `f`, we can suddenly order up to
**nine** overloads! Overload resolution see's the rvalue `int` argument and
tries to find a fitting overload. If there are no immediately matching overloads
for `int` (e.g., they where all not viable), it tries the same implicit
conversion as before, producing an rvalue `long`. That again is tried to be
matched against a fitting overload. If there isn't a fitting one this time, we
go for the least preferred overload, the C-style variadic one. Nice.

This in effect means that we can now remove the `DisableIf` restriction on the
general `print_fizzbuzz` case, leaving us with this:

{% highlight cpp %}
// I put this one on top to show the order we want them invoked in
template<unsigned N, EnableIf<is_multiple_of<N, 15>>...>
void print_fizzbuzz(choice_1){ std::cout << "fizzbuzz\n"; }

template<unsigned N, EnableIf<is_multiple_of<N, 3>>...>
void print_fizzbuzz(choice_2){ std::cout << "fizz\n"; }

template<unsigned N, EnableIf<is_multiple_of<N, 5>>...>
void print_fizzbuzz(choice_3){ std::cout << "buzz\n"; }

template<unsigned N>
void print_fizzbuzz(otherwise){ std::cout << N << "\n"; }
{% endhighlight %}
    
Which looks nice and clean, defining clearly how we want the restrictions to
apply, and without needing to basically invert all restrictions for the general
case. (Yes, I know, the `long` overloads weren't necessary for this case, but
they came for free anyways.)

### Up To <strike>Eleven</strike> Thirteen

So, the obvious question is: can we extend this even more? Yes, abusing not only
*standard conversion sequences* (SCS), like we have until now, but also
involving *user-defined conversion sequences* (UCS). An SCS consists of things
like integer promotion, cv-qualification conversion, array-to-pointer decay,
basically things that work without you needing to do anything. A UCS consists of
an SCS, followed by an implicit user-defined conversion (constructor, conversion
operator), followed by another SCS. We're going to abuse the second SCS here.

{% highlight cpp %}
struct ranker{ operator int(){ return 0; } };
{% endhighlight %}
    
Yep, that's it. Just add another four overloads, netting us a total of
**thirteen**.

{% highlight cpp %}
// Note that all these are *more* preferred than the `int` and `long` ones
// so the numbers on those should be bumped
using choice_1 = ranker&&;
using choice_2 = ranker const&&;
using choice_3 = ranker const volatile&&;
using choice_4 = ranker const&;
{% endhighlight %}

With the call now being `f<15>(ranker{})`. Overload resolution now first tries
to match the `ranker` argument, then tries its implicit (user-defined)
conversion to `int`, then the implicit (standard) conversion from that `int` to
`long`, and at the end, if everything fails, chooses the ellipsis. Awesome.

### A Riddle Wrapped In A Mystery Inside An Enigma

Time to tackle the first SCS. The first SCS is basically used to determine which
conversion operation is going to be performed (i.e., which constructor or
conversion operator is going to be called). If you have a `T&` object for
example, with an `operator U() const;` and are converting to `U`, the first SCS
will consist of a qualification adjustment to `T const&`, after which the
conversion operator will be called.

The first idea that springs to mind might be the following, remembering the
non-`const`/`const` ranking we had before:

{% highlight cpp %}
struct choice_2{};
using choice_3 = int;
using choice_4 = long;

struct choice_1{
  operator choice_2(){ return {}; }
  operator choice_3() const{ return 0; }
};

// for looks
struct select_overload : choice_1{};
{% endhighlight %}
    
With a smaller example:

{% highlight cpp %}
void f(choice_1){}
void f(choice_2){}
void f(choice_3){}
void f(choice_4){}
{% endhighlight %}

And the call being `f(select_overload{})`. Unfortunately, this won't work,
because of <strike>the standard being a sissy</strike> some arcane rules saying
that the first SCS doesn't count for the ordering, unless it's a derived-to-base
conversion. Well, if they say so, we can arrange for that:

{% highlight cpp %}
using choice_3 = int;
using choice_4 = long;

struct choice_2{
  operator choice_3(){ return 0; }
};
struct choice_1 : choice_2{};
{% endhighlight %}
    
With the same overloads and the same invokation, this actually works. If a
`choice_1` can't be matched, it will converted to `choice_2` and thrown into the
matching-pit again. If that fails too, the base class' `operator choice_3()` is
considered, which requires a derived-to-base conversion as the first SCS,
fulfilling the ordering criteria from before. After that, it's normal ranking as
we had before. If we throw qualifications back into the mix, we get a grand
total of **seventeen** overloads.

### But Wait, There's More! 

Astute readers will have noticed that the final ranking code contains the Secret
Ingredient to an even more general solution. It just so turns out that C++ has
another total ordering of conversion built-in &mdash; an infinitely extensible
one to boot. I'm talking about the *derived-to-base* conversion of course, which
we considered a kind of a hinderence last time.

{% highlight cpp %}
template<unsigned I>
struct choice : choice<I+1>{};

// terminate recursive inheritence at a convenient point,
// large enough to cover all cases
template<> struct choice<10>{};

// I like it for clarity
struct select_overload : choice<0>{};
{% endhighlight %}
    
Yep, that's it. Looks deceivingly simple, but is very powerful. Let's adjust our
fizzbuzz and for the kick, add a few extra cases:

{% highlight cpp %}
template<unsigned N, EnableIf<is_multiple_of<N, 15>>...>
void print_fizzbuzz(choice<0>){ std::cout << "fizzbuzz\n"; }

template<unsigned N, EnableIf<is_multiple_of<N, 21>>...>
void print_fizzbuzz(choice<1>){ std::cout << "fizzbeep\n"; }

template<unsigned N, EnableIf<is_multiple_of<N, 27>>...>
void print_fizzbuzz(choice<2>){ std::cout << "fizznarf\n"; }

template<unsigned N, EnableIf<is_multiple_of<N, 35>>...>
void print_fizzbuzz(choice<3>){ std::cout << "buzzbeep\n"; }

template<unsigned N, EnableIf<is_multiple_of<N, 45>>...>
void print_fizzbuzz(choice<4>){ std::cout << "buzznarf\n"; }

template<unsigned N, EnableIf<is_multiple_of<N, 63>>...>
void print_fizzbuzz(choice<5>){ std::cout << "beepnarf\n"; }

template<unsigned N, EnableIf<is_multiple_of<N, 3>>...>
void print_fizzbuzz(choice<6>){ std::cout << "fizz\n"; }

template<unsigned N, EnableIf<is_multiple_of<N, 5>>...>
void print_fizzbuzz(choice<7>){ std::cout << "buzz\n"; }

template<unsigned N, EnableIf<is_multiple_of<N, 7>>...>
void print_fizzbuzz(choice<8>){ std::cout << "beep\n"; }

template<unsigned N, EnableIf<is_multiple_of<N, 9>>...>
void print_fizzbuzz(choice<9>){ std::cout << "narf\n"; }

// we stay with `otherwise` for the general case, so we
// don't have to adjust anything if new overloads are added
template<unsigned N>
void print_fizzbuzz(otherwise){ std::cout << N << "\n"; }
{% endhighlight %}
    
With the magic call being the obvious: `print_fizzbuzz<15>(select_overload{});`.
[And it works, too.][live] Overload resolution is now an obedient little dog
with a cute collar on. Mission accomplished, I'd say.

 [live]: https://ideone.com/IB6tIR

---

*Special thanks go to [Johannes Schaub - litb][litb] for helping me figure out the last part.*
*Thanks to [Luc Danton][luc] for `otherwise`.*
    
 [litb]: http://stackoverflow.com/u/34509
 [luc]: http://stackoverflow.com/u/726300
