---
layout: post
title: Size matters, part 2
---

On [part 1][previous] we saw how two common `std::tuple` implementations could
result in a sub-optimal layout, and how an optimal layout could be achieved.

Now we'll see what an implementation of a tuple with such an optimal layout
actually looks like.

### Overview of an implementation

The main difficulty an implementation needs to overcome is the fact that the
user needs to treat the tuple as if its elements were in the order of the
template arguments: when a call to `get<0>` with a `tuple<int, double>` argument
is made, the result is the `int` even if the tuple was reordered to have the
`double` first.

 [previous]: /2012/07/06/optimal-tuple-i.html "Previously..."
<!-- [next]: /2012/07/13/optimal-tuple-ii.html "To be continued..." -->

