---
layout: doc-ogonek
title: Basic types
---

### code\_point

{% highlight cpp %}
using code_point = char32_t;
{% endhighlight %}

Many algorithms in ogonek operate on sequences of code points. The sequences
themselves can have any type, but their elements must be code points.
`ogonek::code_point` is the type used to represent code points. It is an alias
for the C++ `char32_t` type.

### byte

{% highlight cpp %}
using byte = std::uint8_t;
{% endhighlight %}

Encoding schemes and some encoding forms take input or produce output in the
form of bytes or sequences of bytes. The type used to represent bytes in ogonek
is `ogonek::byte`, which is an alias for the C++ `std::uint8_t` type.

### Ranges

A large portion of ogonek works with sequences of values, notably code points,
code units and bytes. Throughout the library, the Boost range concepts are used
to specify the requirements of input sequences and capabilities of returned
sequences. \[Note: this may be subject to change in the future due to limitations
of the Boost concepts.]

All ranges returned from ogonek algorithms have deferred-evaluation. This means
that nothing happens until the value of the sequence is actually required. If
full materialization of the result is desired, the range can be used to
construct any container that can be constructed from a pair of iterators.

*Example*:
{% highlight cpp %}
auto deferred = ogonek::utf8::decode(source);
// the object holds all information needed to do the decoding when iterated
// but no decoding has actually happened

std::vector<ogonek::codepoint> materialized(deferred.begin(), deferred.end());
// now the range was iterated and the decoded results stored in a vector
{% endhighlight %}

Alternatively, materialisation can be performed using the member function
`materialise`.

*Example*:
{% highlight cpp %}
// vector1 will have type std::vector<ogonek::codepoint>
auto vector1 = ogonek::utf8::decode(source)
                   .materialise<std::vector>();

// vector2 will have type std::vector<std::uint32_t>
auto vector2 = ogonek::utf8::decode(source)
                   .materialise<std::vector<std::uint32_t>>();
{% endhighlight %}

This works for any class or class template with a constructor that accepts two
iterators.
