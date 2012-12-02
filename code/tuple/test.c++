#define MY_STD_TUPLE_LAYOUT_REVERSED // fake!
//#define MY_STD_TUPLE_LAYOUT_STRAIGHT

#include "tuple.h++"

static_assert(std::is_same<my::WithIndices<int, double, int>, std::tuple<my::indexed<int, 0>, my::indexed<double, 1>, my::indexed<int, 2>>>::value,
              "WithIndices must work");

static_assert(my::max<1,5,6,2,4,1,61,32,4,5>::value == 61,
              "max must work");

static_assert(std::is_same<my::sort<my::WithIndices<int, double, int, double, char>>::type, std::tuple<my::indexed<char, 4>, my::indexed<int, 2>, my::indexed<int, 0>, my::indexed<double, 3>, my::indexed<double, 1>>>::value,
              "sorting must work");
int main() {}

