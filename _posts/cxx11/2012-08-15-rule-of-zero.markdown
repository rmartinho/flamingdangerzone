---
layout: post
title: Rule of Zero
categories: cxx11
short: where I rediscover the Single Responsibility Principle
---

### Ownership

In C++ the destructors of objects with automatic storage duration are invoked
whenever their scope ends. This property is often used to handle cleanup of
resources automatically in a pattern known by the not very descriptive name RAII.

An essential part of RAII is the concept of resource ownership: the object
responsible for cleaning up a resource in its destructor *owns that resource*.
Proper ownership policies are the secret to avoid resource leaks.

Implementing ownership policies can be tedious and error-prone work. Many
articles and even books have been written to provide guidance and rules of thumb
to help programmers with that task.

The good thing about C++ is that it allows us to encapsulate those ownership
policies into generic reusable classes. This can greatly reduce the effort of
handling ownership in other classes.

### Value semantics

When we pass objects by value in C++, they get copied. If we want to store a
value as a member, we use a non-pointer object member, to avoid running into
dangling references immediately. Whenever we want a function to handle a copy of
an argument, we can take that argument by value. If we want to handle the same
object without making a copy, we can take that argument by reference (i.e.  with
a reference type).

Using value semantics is essential for RAII, because references don't affect the
lifetime of their referrents. Unfortunately, the default value semantics don't
interact very well with the idea of ownership described above.

### Rule of Three

What happens when a copy of an object that owns a resource is made? By default a
memberwise copy is performed. This means that the internal resource handle the
object stores will be copied, ending up with two objects that own the same
resource. This is likely to lead to them both trying to release the resource,
and catastrophe follows.

Fortunately, C++ allows programmers to define what value semantics means for a
class, by providing user-defined copy constructors and copy assignment
operators.

To get consistent and correct behaviour, one has to pay close attention to how
these user-defined functions interoperate. This idea is commonly known as the
[*Rule of Three*][rule of three]:

> If a class defines a destructor, copy constructor or copy assignment operator then
> it should probably explicitly define all three, and not rely on their default
> implementation.

To solve this conflict between value semantics and ownership, one can take
several approaches. Common choices include:

1. do not allow copies to be made;
2. provide custom copy semantics that copy the resource;
3. make both objects have ownership of the resource;
4. make copying transfer the resource;

Option #1 gives exclusive ownership of a resource to a single object and does not
allow that ownership to be transferred to another object. This is quite
limiting since it prevents passing or returning those resources by value, and
requires the use of pointers or references instead. Making it a member of
another class also makes that class non-copyable, even if copies are desirable.
An example of a class with this policy is [`boost::scoped_ptr`][scoped_ptr].

Option #2 also gives exclusive ownership to a single object, but emulates the
"normal" value semantics for the resource. This is useful for polymorphic
classes to avoid the slicing problem, and for other resources for which a copy
can be made easily (POSIX file descriptors is a possible example, since they can
be copied with `dup`).

Option #3 shares the ownership between the two objects. This one can be quite
tricky to implement, especially in a thread-safe manner. Common implementations
maintain a reference count that tracks how many instances own the resource, and
delete it when it reaches zero. Both Boost and the latest standard provide this
policy in the form of [`shared_ptr`][shared_ptr].

Option #4 gives exclusive ownership but allows that ownership to be transferred.
This is what [`std::auto_ptr`][auto_ptr] does, and it is known for not being a particularly
good approach (in fact, `std::auto_ptr` is now a deprecated feature).

Obviously, many more reasonable ownership policies can be invented.

### Rule of Five

Come C++11 there are some extra options for our ownership policies. With move
semantics in the mix, we can now provide options for transferring ownership
between objects that are safer than option #4 above.

Option #1 can be improved to allow transferring ownership by adding a move
constructor and a move assignment operator. This makes it a lot more usable and
a lot more useful. It can be returned from functions and passed by value to
transfer ownership. An example of this is [`std::unique_ptr`][unique_ptr], the first choice
when one needs a smart pointer.

Option #2 can also benefit from having the move special members, since it avoids
unneeded copies. In option #3 it gives the ability to transfer ownership
without the need to update reference counts. 

Option #4 was not very good to start with. Since it basically boils down to an
attempt at move semantics with the copy special members, option #1 is
preferrable. This is the reason `std::auto_ptr` was deprecated in favour of
`std::unique_ptr`.

### Rule of Zero

The rule of three and a solid understanding of the different ownership
strategies are essential to writing well-behaved classes and programs.  However,
getting it right is often tricky and can soon become overwhelming, if you try to
go solely by these rules of thumb. Fixating too much on them can make one miss
the forest for the trees. 

I'd go so far as to claim that **these rules are rarely needed**. Why is that?
Well, as I mentioned in the introduction on ownership, C++ allows us to
encapsulate ownership policies into generic reusable classes. *This* is the
important bit! Most often, our ownership needs can be catered for by
"ownership-in-a-package" classes.

Common "ownership-in-a-package" classes are included in the standard library:
`std::unique_ptr` and `std::shared_ptr`. Through the use of custom deleter
objects, both have been made flexible enough to manage virtually any kind of
resource.

As an example, let's write a class that owns a `HMODULE` resource from the
Windows API. A `HMODULE` (a type alias for `void*`) is a handle to a loaded
module, usually a DLL.  It must be released by passing it to the `FreeLibrary`
function.

One option would be to implement a class that follows the rule of three (or
five).

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
    // copy constructor is implicitly forbidden due to user-defined move constructor

    // move assignment
    module& operator=(module&& that) {
        module copy { std::move(that) };
        std::swap(handle, copy.handle);
        return *this;
    }
    // copy assignment is implicitly forbidden due to user-defined move assignment

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
class module {
public:
    explicit module(std::wstring const& name)
    : handle { ::LoadLibrary(name.c_str()), &::FreeLibrary } {}

    // other module related functions go here

private:
    using module_handle = std::unique_ptr<void, decltype(&::FreeLibrary)>;

    module_handle handle;
};
{% endhighlight %}

Believe it or not, this does exactly the same thing as the first version, but
all lifetime members have been implicitly defined by the compiler. This has
several advantages:

 - No duplication of the ownership logic;
 - No mixing of ownership with other concerns;
 - Less code, less opportunity for error; no code to review/change when the
   class changes;
 - And the kicker is, we got exception safety *for free*.

To illustrate the last bullet, consider a class that owns two resources. How
would the constructor be written in an exception-safe manner?

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

If the construction of `resource_two` throws, the `resource_one` needs to be
cleaned up. This can certainly be done, but it further complicates the code and
is easily overlooked.

Instead, if we reuse ownership policies (e.g. by using `std::unique_ptr<foo>`
instead of `foo*`), the cleanup will be automatic even in the face of exceptions
during construction.

If we want some ownership policy that the existing classes don't provide, we can
just write a new "ownership-in-a-package" class and reap the same benefits.
The code will have to deal with that ownership somewhere, and having it isolated
in a separate place makes it easier to maintain.

So, we have arrived at the Rule of Zero (which is actually a particular
instance of the *Single Responsibility Principle*):

> Classes that have custom destructors, copy/move constructors or copy/move
> assignment operators should deal exclusively with ownership. Other classes
> should not have custom destructors, copy/move constructors or copy/move
> assignment operators.

---

*Special thanks to [sehe][sehe] for taking the time for a thorough review of
this post and several suggestions*

 [rule of three]: http://stackoverflow.com/q/4172722/46642
 [scoped_ptr]: http://www.boost.org/libs/smart_ptr/scoped_ptr.htm
 [auto_ptr]: http://en.cppreference.com/w/cpp/memory/auto_ptr
 [shared_ptr]: http://en.cppreference.com/w/cpp/memory/shared_ptr
 [unique_ptr]: http://en.cppreference.com/w/cpp/memory/unique_ptr
 [sehe]: http://stackoverflow.com/u/85371

