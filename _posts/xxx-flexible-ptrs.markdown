---
layout: post
title: On the flexibility of the standard smart pointers
---

On the article on the [Rule of Zero][rule of zero], I mentioned that the C++ standard library includes two ownership-in-a-package classes: `unique_ptr` and `shared_ptr`. I also mentioned that these two are very flexible and can be used to handle almost any resource by using custom deleters. In this article I will explore this flexibility, sometimes in ways that are not obvious on first sight.

### The defaults

By default, `unique_ptr<T>` uses [`default_delete<T>`][default_delete]  as the deleter. This default deleter simply releases a dynamically allocated object. The template is specialized so that it uses the right choice of `delete` or `delete[]`.

`shared_ptr` uses `delete` by default. There was talk within the standards committee of [a specialization for arrays][shared_ptr array] but it was not standardized due to lack of time and low priority. It can still be used for arrays with a custom deleter and some awkward syntax, but [`boost::shared_array`][shared_array] is a better choice for that.

### 

 [rule of zero]: /2012/08/15/rule-of-zero.html
 [default_delete]: http://en.cppreference.com/w/cpp/memory/default_delete
 [shared_ptr array]: http://stackoverflow.com/a/8947700/46642
 [shared_array]: http://www.boost.org/libs/smart_ptr/shared_array.htm