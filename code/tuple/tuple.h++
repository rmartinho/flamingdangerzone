// Optimal layout tuple
//
// Written in 2012 by Martinho Fernandes <martinho.fernandes@gmail.com>
//
// To the extent possible under law, the author(s) have dedicated all copyright and related
// and neighboring rights to this software to the public domain worldwide. This software is
// distributed without any warranty.
//
// You should have received a copy of the CC0 Public Domain Dedication along with this software.
// If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.

#ifndef MY_TUPLE_HPP
#define MY_TUPLE_HPP

#include <tuple>
#include <type_traits>

#include <cstddef>

namespace my {
    template <std::size_t Size, std::size_t Align = Size>
    using layout = typename std::aligned_storage<Size, Align>::type;

    template <std::size_t I, typename T>
    using TupleElement = typename std::tuple_element<I, T>::type;

    template <std::size_t I>
    using index = std::integral_constant<std::size_t, I>;
    template <std::size_t... I>
    using indices = std::tuple<index<I>...>;

    template <typename... T>
    class optimal_order;

    template <typename... T>
    using OptimalMap = typename optimal_order<T...>::map;
    template <typename... T>
    using ReverseOptimalMap = typename optimal_order<T...>::reverse_map;

    template <typename T>
    struct member { T _; };

    template <typename T, typename U>
    struct layout_before_impl
    : std::integral_constant<bool, (alignof(member<T>) > alignof(member<U>))> {};

    template <typename T, typename U>
    struct layout_before
    #if defined(MY_STD_TUPLE_LAYOUT_STRAIGHT)
    : layout_before_impl<T,U> {};
    #elif defined(MY_STD_TUPLE_LAYOUT_REVERSED)
    : layout_before_impl<U,T> {};
    #endif
} // namespace my

#endif // MY_TUPLE_HPP

