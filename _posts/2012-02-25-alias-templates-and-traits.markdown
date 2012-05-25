---
layout: post
title: Alias templates and traits
---

<h3>Getting rid of typename</h3>

<p>Anyone that has ever tried to do some template meta-programming, has likely run into the need for the <a href="http://stackoverflow.com/a/613132/46642"><code>typename</code> keyword</a>. Using type traits often involves this keyword.</p>

{{ highlight cpp }}
    // examples
    typename std::remove_const<T>::type
    typename std::add_lvalue_reference<T>::type
    typename std::conditional<std::is_const<T>::value, typename std::add_const<U>::type, T>::type
{{ endhighlight }}

<p>This makes for verbose code and quickly becomes annoying. Fortunately C++11 brings us template aliases (available on Clang 3.0 and GCC 4.7). With these one can make these things much less of an eye sore.</p>

<p>One can capture this <code>typename T::type</code> pattern with a template alias that can be used to replace it:</p>

{{ highlight cpp }}
    template <typename T>
    using Invoke = typename T::type;
{{ endhighlight }}

And rewrite the previous examples as:

{{ highlight cpp }}
    Invoke<std::remove_const<T>>
    Invoke<std::add_lvalue_reference<T>>
    Invoke<std::conditional<std::is_const<T>::value, Invoke<std::add_const<U>>, T>>
{{ endhighlight }}

<p>This already helps, but one can also go one step further by making specialised aliases for each trait.</p>

{{ highlight cpp }}
    template <typename T>
    using RemoveConst = Invoke<std::remove_const<T>>
    template <typename T>
    using AddLvalueReference = Invoke<std::add_lvalue_reference<T>>
    template <typename T>
    using AddConst = Invoke<std::add_const<U>::type
    template <typename If, typename Then, typename Else>
    using Conditional = Invoke<std::conditional<If::value, Then, Else>>
{{ endhighlight }}

<p>And the previous examples becomes a lot more readable:</a>

{{ highlight cpp }}
    RemoveConst<T>
    AddLvalueReference<T>
    Conditional<std::is_const<T>, AddConst<U>, T>>
{{ endhighlight }}

<p>It almost doesn't look like TMP at all.</p>

