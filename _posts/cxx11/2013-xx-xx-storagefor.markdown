---
layout: post
title: Messing around with raw storage
categories: cxx11
short: where ?
published: false
---



{% highlight cpp %}
template <typename T>
class uninitialized {
public:

    template <typename... Args>
    T& initialize(Args&&... args) {
        return *::new(&storage) T(std::forward<Args>(args)...);
    }

    T& get() { return storage; }
    T const& get() const { return storage; }

    void uninitialize() {
        storage.~T();
    }
private:
    union { T storage; };
}
{% endhighlight %}

