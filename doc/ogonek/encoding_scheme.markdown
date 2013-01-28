---
layout: doc-ogonek
title: Encoding schemes
---

Encoding schemes are similar to [encoding forms], but they specify ways to
serialise code points into bytes instead of some particular code unit. Encoding
schemes shall provide the same interface as encoding forms, but the `code_unit`
nested type shall have size 1.

Ogonek includes the following encoding schemes.

- [UTF-16BE] as `utf16be` in `<ogonek/encoding/utf16be.h++>`;
- [UTF-16LE] as `utf16le` in `<ogonek/encoding/utf16le.h++>`;
- [UTF-32BE] as `utf32be` in `<ogonek/encoding/utf32be.h++>`;
- [UTF-32LE] as `utf32le` in `<ogonek/encoding/utf32le.h++>`.

In addition, any encoding form with a byte-sized code unit can work as an
encoding scheme.

#### encoding\_scheme

The class template `encoding_scheme` can be used to define encoding schemes by
combining an encoding form with a byte order. The two byte orders supported are
represented by the types `big_endian` and `little_endian`. These three types are
in the header `<ogonek/encoding/encoding_scheme.h++>`.

*Example*:
{% highlight cpp %}
using utf16be = ogonek::encoding_scheme<ogonek::utf16, ogonek::big_endian>;
{% endhighlight %}

---

### See Also

- [Encoding forms][encoding forms]
- [Validation][validation]

 [validation]: validation.html
 [encoding forms]: encoding_form.html
 [UTF-16BE]: http://en.wikipedia.org/wiki/UTF-16BE
 [UTF-16LE]: http://en.wikipedia.org/wiki/UTF-16LE
 [UTF-32BE]: http://en.wikipedia.org/wiki/UTF-32BE
 [UTF-32LE]: http://en.wikipedia.org/wiki/UTF-32LE
