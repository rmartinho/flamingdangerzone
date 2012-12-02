#define MY_STD_TUPLE_LAYOUT_REVERSED // fake!
//#define MY_STD_TUPLE_LAYOUT_STRAIGHT

#include "tuple.h++"

static_assert(!my::layout_before<my::layout<4>, my::layout<2>>::value,
              "in a reversed tuple 4-aligned does not come before 2-aligned");
static_assert(my::layout_before<my::layout<2>, my::layout<4>>::value,
              "in a reversed tuple 2-aligned comes before 4-aligned");

// assuming references implemented as pointers and pointers are 4- or 8-aligned
static_assert(!my::layout_before<int&, my::layout<2>>::value,
              "in a reversed tuple references do not come before 2-aligned");
static_assert(my::layout_before<my::layout<2>, my::layout<4>>::value,
              "in a reversed tuple 2-aligned comes before references");

int main() {}

