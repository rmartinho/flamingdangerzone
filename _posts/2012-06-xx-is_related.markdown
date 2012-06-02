---
layout: post
title: How to write non-copy constructors
---

Let us say we are writing a scope guard class for use in C++11.

{% highlight cpp %}
template <typename Fun>
class guard {
public:
    template <typename U,
              DisableIf<is_related<guard, U>>...>
    explicit guard(U&& u) : fun(std::forward<U>(u)) {}

    guard(guard&& that)
    : on(that.on)
    , fun(std::move(that.fun)) {
        that.on = false;
    }

    ~guard() {
        if(on) fun();
    }
private:
    bool on = true;
    Fun fun;
};
{% endhighlight %}

