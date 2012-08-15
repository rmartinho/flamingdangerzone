---
layout: post
title: Rule of Zero
---

### Ownership

In C++ the destructors of objects with automatic storage duration are invoked
whenever their scope ends. In C++ this property is often used to handle cleanup
of resources automatically in a pattern known as *Resource Acquisition Is
Initialisation* (RAII).

An essential part of RAII is the concept of resource ownership: the object
responsible for cleaning up a resource in its destructor *owns that resource*.
Proper ownership policies are the secret to avoid resource leaks. And C++ allows
us to encapsulate ownership policies into generic reusable classes.

### Value semantics

When we use object types (as opposed to reference types) to pass values around
in C++, those values get copied. If we want to store a value as a member, it
should be of a non-pointer object type, otherwise it's ripe ground for dangling
references. Whenever we want a function to handle a copy of an argument, we can
take that argument by value (i.e. with an object type). If we want to handle the
same object without making a copy, we can take that argument by reference (i.e.
with a reference type).

Using value semantics (i.e. object types) is important for RAII, because
references don't affect the storage duration to their referrents. Unfortunately,
by default these semantics don't interact very well with the idea of ownership
described above.

### Rule of Three

What happens when a copy of an object that owns a resource is made? By default a
memberwise copy is performed. This means that the internal resource handle the
object stores will be copied, ending up with two objects that own the same
resource. Ultimately they'll both try to release the resource, and that will
lead to catastrophe.

Luckily, C++ allows programmers to define what value semantics means for a
class, by allowing user-defined copy constructors and copy assignment
operators.

This idea is commonly known as the [*Rule of Three*][rule of three]:

> If a class defines a destructor, constructor or copy assignment operator then
> it should explicitly define all three and not rely on their default
> implementation.

To solve this conflict between value semantics and ownership, one can take
several options. Common choices include:

1. do not allow copies to be made;
 
2. provide custom copy semantics that copy the resource;

3. make both objects have ownership of the resource;

4. make copying transfer the resource;

Option #1 gives exclusive ownership of a resource to a single object and does not
allow that ownership to be transferred to another object. This is quite
limiting since it prevents passing or returning those resources by value, and
requires the use of pointers or references instead. Making it a member of
another class also makes that class non-copyable, even if copies are desirable.
This is what is used in `boost::scoped_ptr`.

Option #2 also gives exclusive ownership to a single object, but acts as if the
resource had "normal" value semantics. This is useful for polymorphic classes to
avoid the slicing problem, and for other resources for which a copy can be made
easily (POSIX file descriptors is a possible example, since they can be copied
with `dup`).

Option #3 shares the ownership between the two objects. This one can be quite
tricky to implement, especially in a thread-safe manner. Usual implementations
maintain a reference count that tracks how many instances own the resource, and
delete it when it reaches zero. Both Boost and the latest standard provide this
policy in the form of `shared_ptr`.

Option #4 gives exclusive ownership but allows that ownership to be transferred.
This is what `std::auto_ptr` does, and it is known for not being a particularly
good approach (in fact, `std::auto_ptr` is now a deprecated feature).

And obviously, any other reasonable ownership policy one can think of can be
implemented.

### Rule of Five

Come C++11 and there are some extra options for our ownership policies. With
move semantics in the mix, we can now provide options for transferring ownership
between objects that are safer than option #4 above.

Option #1 can be improved to allow transferring ownership by adding a move
constructor and a move assignment operator. This makes it a lot more usable, but
also a lot more useful. It can be returned from functions and passed by value to
transfer ownership. An example of this is `std::unique_ptr`, the first choice
when one needs a smart pointer.

Option #2 can also benefit from having the move special members, since it avoids
unneeded copies. And in option #3 it gives the ability to transfer ownership
without the need to update reference counts. 

Option #4 was not very good to start with, and since it basically boils down to
implementing move semantics in the copy special members option #1 is
preferrable. This is the reason `std::auto_ptr` was deprecated in favour of
`std::unique_ptr`.

### Rule of Zero

Those rules of thumb are quite important for writing well-behaved programs.
However, fixating too much on them can make one not see the forest for the
trees. I'd go so far as claiming that these rules are rarely needed.

And why is that? Because, as I said when describing the idea of ownership, C++
allows us to encapsulate ownership policies into generic reusable classes.
*This* is the important bit. Many times our ownership needs can be provided by
"ownership-in-a-package" classes.

The most common "ownership-in-a-package" classes are probably the ones included
in the standard library: `std::unique_ptr` and `std::shared_ptr`. Both let the
user provide custom deleter objects, allowing to use them for ownership of most
kinds of resources.

Consider for example, writing a class that owns a `HMODULE` resource from the
Windows API. A `HMODULE` (a type alias for `void*`) is a handle to a loaded DLL
or executable image (collectively known as modules). It must be released by
passing it to the `FreeLibrary` function.

One option would be to implement a class that follows the above rule of three
(or five).

{% highlight cpp %}
class module {
public:
    explicit module(std::wstring const& name)
    : handle { ::LoadLibrary(name.c_str()) } {}

    // move constructor
    module(module&& that)
    : handle { that.handle } {
        that.handle = nullptr;
    }
    // copy constructor is implicitly forbidden due to explicit move constructor

    // move assignment
    module& operator=(module&& that) {
        module copy { std::move(that) };
        std::swap(handle, copy.handle);
        return *this;
    }
    // copy assignment is implicitly forbidden due to explicit move assignment

    // destructor
    ~module() {
        ::FreeLibrary(handle);
    }

    // other module related functions go here

private:
    HMODULE handle;
};
{% endhighlight %}

But why would anyone want to do that when you can just grab an existing reusable
ownership policy instead and get the same effect?

{% highlight cpp %}
using module_handle = std::unique_ptr<void, decltype(&::FreeLibrary)>;
module_handle make_module_handle(HMODULE h) {
    return module_handle { h, &::FreeLibrary }; // custom deleter
}

class module {
public:
    explicit module(std::wstring const& name)
    : handle { make_module_handle(::LoadLibrary(name.c_str())) } {}

    // all lifetime members are defined implicitly

    // other module related functions go here

private:
    module_handle handle;
};
{% endhighlight %}

This does the same thing as the previous version, but it doesn't duplicate the
code for the ownership policy: it lets the compiler handle that.

For another advantage of not mixing ownership with other concerns, consider a
class that owns two resources. How would the constructor be written in an
exception-safe manner? If the constructor of the second one fails, the first one
needs to be cleaned up.

{% highlight cpp %}
struct something {
    something()
    : resource_one { new foo }
    , resource_two { new foo } // what if this throws?
    {}
    // blah
    foo* resource_one;
    foo* resource_two;
};
{% endhighlight %}

If we reuse ownership policies instead (e.g. by using `std::unique_ptr<foo>`
instead of `foo*`), the first resource will be automatically cleaned up if
construction of the second fails. And for this safety we pay the price of
actually writing less code.

If we want some ownership policy that the existing classes don't provide, we can
just write a new "ownership-in-a-package" class and reap the same benefits.
The code will have to handle that ownership somewhere, and having it alone in a
separate place makes it easier to maintain.

The rule of zero, which is actually a particular instance of the *Single
Responsibility Principle*, is thus:

> Classes that have custom destructors, copy/move constructors or assignment
> operators should deal exclusively with ownership. Other classes should not
> have custom destructors, copy/move constructors or assignment operators.

 [rule of three]: http://stackoverflow.com/q/4172722/46642

