---
layout: doc-ogonek
title: Encoding forms
---

Each encoding form specifies how a code point is expressed as a sequence of one
or more code units. In ogonek these are realised as classes with static
functions for encoding and decoding.

Each encoding form shall provide the following members.

---

#### code\_unit

The type of the code unit that the encoding form uses. This must be an integral
type.

*Example*:
{% highlight cpp %}
std::vector<ogonek::utf8::code_unit> utf8_data;
{% endhighlight %}

---

#### is\_fixed\_width

A `bool` constant expression with value `true` if all code points are encoded
using the same number of code units; or `false` otherwise.

*Example*:
{% highlight cpp %}
static_assert(!ogonek::utf8::is_fixed_width,
              "UTF-8 does not have fixed width");
{% endhighlight %}

---

#### max\_width

A `std::size_t` constant expression with value equal to the maximum number of
code units required to encode any Unicode code point.

*Example*:
{% highlight cpp %}
static_assert(ogonek::utf16::max_width == 2,
              "UTF-16 uses at most 2 code units per code point");
{% endhighlight %}

---

#### is\_self\_synchronising

A `bool` constant expression with value `true` if the start of a multiple code
unit sequence can be found in constant time; or `false` otherwise.

*Example*:
{% highlight cpp %}
static_assert(ogonek::utf8::is_self_synchronising,
              "UTF-8 is self-synchronising");
{% endhighlight %}

---

#### state

A trivial type that can hold any state needed to encode or decode a sequence
with this encoding form. Value-initialising an instance of this type shall
produce the initial state for any conversion.

This shall be an empty type if the encoding form is stateless.

*Example*:
{% highlight cpp %}
static_assert(std::is_empty<ogonek::utf8::state>(),
              "UTF-8 is stateless");
{% endhighlight %}

---

#### encode

A static function template that accepts as arguments any sequence of code points
and a [validation strategy][validation], and returns the sequence of code units
corresponding to the given sequence of code points. Invalid data is handled
according to the validation strategy given.

*Example*:
{% highlight cpp %}
auto encoded = ogonek::utf8::encode(decoded, ogonek::discard_errors)
                   .materialise<std::vector>();
{% endhighlight %}

---

#### decode

A static function template that accepts as arguments any sequence of code units
and a validation strategy, and returns the sequence of code points corresponding
to the given sequence of code units. Invalid data is handled according to the
validation strategy given.

*Example*:
{% highlight cpp %}
auto decoded = ogonek::utf8::decode(encoded, ogonek::discard_errors)
                   .materialise<std::vector>();
{% endhighlight %}

---

#### encode\_one

A static function template that accepts as arguments a code point, an lvalue
reference to the current encoding state, and a validation strategy, and returns
the sequence of code units corresponding to the given code point. The sequence
returned is materialised. It shall update the encoding state according to the
codepoint that was encoded. Invalid data is handled according to the validation
strategy given. 

*Example*:
{% highlight cpp %}
ogonek::utf8::state s{};
auto encoded = ogonek::utf8::encode_one(u, s, ogonek::discard_errors)
{% endhighlight %}

---

#### decode\_one

A static function template that accepts as arguments any sequence of code units,
an lvalue reference to a `code_point`, an lvalue reference to the current
encoding state, and a validation strategy. This function shall decode the first
code point from the given sequence, store that result in the second argument,
and update the encoding state accordingly. It shall return the remaining
sequence of code units, without the code units that where decoded. Invalid data
is handled according to the validation strategy given.

*Example*:
{% highlight cpp %}
ogonek::utf8::state s{};
ogonek::code_point result;
auto remaining = ogonek::utf8::decode_one(source, result, s, ogonek::discard_errors);
{% endhighlight %}

<!-- TODO: flush state -->

---

#### replacement\_character

This member is optional if the encoding form can use U+FFFD
&#640;&#7431;&#7448;&#671;&#7424;&#7428;&#7431;&#7437;&#7431;&#628;&#7451;
&#7428;&#668;&#7424;&#640;&#7424;&#7428;&#7451;&#7431;&#640; as a replacement
character. If present, it shall be a `code_point` constant expression and its
value shall be an adequate replacement character for the encoding form.

---

Ogonek includes the following encoding forms:

- [UTF-8] as `utf8` in `<ogonek/encoding/utf8.h++>`;
- [UTF-16] as `utf16` in `<ogonek/encoding/utf16.h++>`;
- [UTF-32] as `utf32` in `<ogonek/encoding/utf32.h++>`;
- [ASCII] as `ascii` in `<ogonek/encoding/ascii.h++>`;
- [Latin-1] as `latin1` in `<ogonek/encoding/latin1.h++>`;
- [Windows-1252] as `windows1252` in `<ogonek/encoding/windows1252.h++>`;
- [GB18030] as `gb18030` in `<ogonek/encoding/gb18030.h++>`;
- and others.

---

### See Also

- [Validation][validation]
- [Encoding schemes]

 [validation]: validation.html
 [Encoding schemes]: encoding_scheme.html
 [UTF-8]: http://en.wikipedia.org/wiki/UTF-8
 [UTF-16]: http://en.wikipedia.org/wiki/UTF-16
 [UTF-32]: http://en.wikipedia.org/wiki/UTF-32
 [ASCII]: http://en.wikipedia.org/wiki/ASCII
 [Latin-1]: http://en.wikipedia.org/wiki/Latin-1
 [Windows-1252]: http://en.wikipedia.org/wiki/Windows-1252
 [GB18030]: http://en.wikipedia.org/wiki/GB18030
