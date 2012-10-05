---
layout: post
title: unique_ptr is very flexible
---

In the article on the [Rule of Zero][rule of zero], I mentioned that the C++
standard library includes two ownership-in-a-package classes: `unique_ptr` and
`shared_ptr`. I also mentioned that these two are very flexible and can be used
to handle almost any resource by means of custom deleters. There is a great
write-up about the [flexibility of `shared_ptr`][boost sp_techniques] in the
boost documentation. In this article I will explore some of the flexibility of
`unique_ptr`.

### The customization hook

By default, `unique_ptr<T>` uses [`default_delete<T>`][default_delete]  as the
deleter. This default deleter simply releases a dynamically allocated object.
The template is specialized so that it uses the right choice of `delete` or
`delete[]`. This covers objects allocated with both `new` and `new[]`, which
are the most common kind of resource that needs cleanup.

However, not all resources that need cleanup are allocated with `new` or
`new[]`. For those `unique_ptr` allows the use of a custom deleter. Unlike with
`shared_ptr`, however, this custom deleter is part of the type. This is so in
order to keep the zero-overhead promise given by `unique_ptr`.

### Legacy C-like resources

The custom deleter can be any callable that takes a pointer argument and
disposes of the associated resource. An example I've shown in the previous
article was a `unique_ptr` that took care of disposal of handles for Windows
modules.

{% highlight cpp %}
using module_handle = unique_ptr<void, decltype(&::FreeLibrary)>;
{% endhighlight %}
    
We could do something similar to that for other resources that are represented
with pointers, like, for example, `FILE*` from the C library.

{% highlight cpp %}
using c_file = unique_ptr<FILE, decltype(&::fclose)>;
{% endhighlight %}
    
This approach requires passing the cleanup function as argument to the
constructor of the `unique_ptr`.

### Type erased deleter

One property of `unique_ptr` that may be seen as a flaw in some scenarios, is
the fact that when used on an interface, the type of the deleter becomes part
of that interface. This potential flaw exists because `unique_ptr` promises
zero-overhead over a naked pointer. However, if one is willing to accept some
overhead to avoid this flaw, one can still do that.

Basically, we would need some way to erase to the type of the deleter, like
`shared_ptr` does. Luckily the standard library includes components that
provide some type erasure functionality. In this case, we can use
`std::function`'s ability to erase the deleter's type.

{% highlight cpp %}
template <typename T>
using exclusive_ptr =  std::unique_ptr<T, std::function<void(T*)>>;
{% endhighlight %}

Now the real type of the deleter is not exposed by the interface. With this
type-erased variant we can change the deleter without changing any interfaces,
just like we can with `shared_ptr`.

### Non-pointer handles

An ability of `unique_ptr` that is not obvious at first sight is the ability to
handle resources that are not represented by an actual pointer. As an example
of such a resource would be a POSIX file descriptor, which is represented
simply by an `int`.

How can `unique_ptr` handle POSIX file descriptors, then? The key is in the
[member types of `unique_ptr`][unique_ptr member types]. The member `pointer`,
which governs what is stored in the `unique_ptr`, does not have to be an actual
pointer. It is defined to be `std::remove_reference<D>::type::pointer` if that
type exists, otherwise `T*`, for some `unique_ptr<T, D>`.

That means that our customization hook, the deleter, actually decides what type
represents the resource. To make a `unique_ptr` handle a POSIX file descriptor,
we need to provide a deleter with the appropriate `pointer` member type.

{% highlight cpp %}
struct file_descriptor_deleter {
    using pointer = int;

    void operator()(pointer fd) const {
        ::close(fd);
    }
};
{% endhighlight %}

using file_descriptor = std::unique_ptr<void, file_descriptor_deleter>;

Notice how the first argument of the `unique_ptr` template is `void`. Since
this custom deleter is dictating what the handle is, the first argument is not
relevant (it is only used when determining the default handle type). It could
be anything else, but `void` is fine.

Sadly, this is not enough. The `pointer` member of the deleter must satisfy the
`NullablePointer` requirements and `int` doesn't.

 [unique_ptr member types]: http://en.cppreference.com/w/cpp/memory/unique_ptr#Member_types
 [rule of zero]: /2012/08/15/rule-of-zero.html
 [default_delete]: http://en.cppreference.com/w/cpp/memory/default_delete
 [boost sp_techniques]: http://www.boost.org/libs/smart_ptr/sp_techniques.html

