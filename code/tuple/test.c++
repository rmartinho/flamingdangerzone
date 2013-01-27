//#define MY_STD_TUPLE_LAYOUT_REVERSED // fake!
#ifndef MY_STD_TUPLE_LAYOUT_REVERSED
#define MY_STD_TUPLE_LAYOUT_STRAIGHT
#endif

#include "tuple.h++"

#include <iostream>
#include <cassert>

int main() {
    my::tuple<int, double, float> t1(1,2,3);
    my::tuple<int, int, int> t2 = t1;

    my::get<0>(t1) = 4;
    my::get<1>(t1) = 3.2;
    my::get<2>(t1) = 1.2f;

    assert(my::get<0>(t1) == 4);
    assert(my::get<1>(t1) == 3.2);
    assert(my::get<2>(t1) == 1.2f);
}

