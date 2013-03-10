---
layout: post
title: Playing with raw storage
categories: cxx11
short: where ?
published: false
---

{% highlight cpp %}
template <typename T>
class uninitialized {
public:

    template <typename... Args>
    void initialize(Args&&... args) {
        ::new(&storage) T(std::forward<Args>(args)...);
    }

    void uninitialize() {
        storage.~T();
    }
private:
    union { T storage; };
}
{% endhighlight %}

