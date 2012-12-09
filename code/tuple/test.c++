//#define MY_STD_TUPLE_LAYOUT_REVERSED // fake!
#ifndef MY_STD_TUPLE_LAYOUT_REVERSED
#define MY_STD_TUPLE_LAYOUT_STRAIGHT
#endif

#include "tuple.h++"

static_assert(std::is_same<my::MapToStorage<my::layout<1>, my::layout<4>, my::layout<2>>, my::indices<2, 0, 1>>::value,
              "map to storage must be computed correctly");

static_assert(std::is_same<my::MapToInterface<my::layout<1>, my::layout<4>, my::layout<2>>, my::indices<1, 2, 0>>::value,
              "map to interface must be computed correctly");

static_assert(std::is_same<my::OptimalStorage<my::layout<1>, my::layout<4>, my::layout<2>>, std::tuple<my::layout<4>, my::layout<2>, my::layout<1>>>::value,
              "optimal storage must be computed correctly");

using indexed_test = my::WithIndices<my::Sort<my::WithIndices<std::tuple<int, double, float, char>>>>;

#include <iostream>

int main() {
    std::cout << my::find_index<0, indexed_test>::value << '\n';
}

