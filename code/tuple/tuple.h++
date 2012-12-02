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
    // utils
    template <typename T>
    struct identity {
        using type = T;
    };

    template <std::size_t Size, std::size_t Align = Size>
    using layout = typename std::aligned_storage<Size, Align>::type;

    template <bool B>
    using Bool = std::integral_constant<bool, B>;

    template <typename Condition, typename Then, typename Else>
    using Conditional = typename std::conditional<Condition::value, Then, Else>::type;

    template <std::size_t I, typename T>
    using TupleElement = typename std::tuple_element<I, T>::type;

    template <std::size_t Acc, std::size_t... Tail>
    struct max;
    template <std::size_t Acc>
    struct max<Acc> : std::integral_constant<std::size_t, Acc> {};
    template <std::size_t Acc, std::size_t Head, std::size_t... Tail>
    struct max<Acc, Head, Tail...>
    : std::conditional<Acc < Head, max<Head, Tail...>, max<Acc, Tail...>>::type {};

    // code starts here
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

    template <typename T, std::size_t I>
    struct indexed {
        using type = T;
        static constexpr auto i = I;
    };

    template <typename Acc, typename... T>
    struct with_indices_impl : identity<Acc> {};
    template <typename... Acc, typename Head, typename... Tail>
    struct with_indices_impl<std::tuple<Acc...>, Head, Tail...>
    : with_indices_impl<std::tuple<Acc..., indexed<Head, sizeof...(Acc)>>, Tail...> {};

    template <typename... T>
    struct with_indices : with_indices_impl<std::tuple<>, T...> {};
    template <typename... T>
    using WithIndices = typename with_indices<T...>::type;

    template <typename Head, typename Tail>
    struct cons;
    template <typename Head, typename... Tail>
    struct cons<Head, std::tuple<Tail...>>
    #if defined(MY_STD_TUPLE_LAYOUT_STRAIGHT)
    : identity<std::tuple<Tail..., Head>> {};
    #elif defined(MY_STD_TUPLE_LAYOUT_REVERSED)
    : identity<std::tuple<Head, Tail...>> {};
    #endif
    template <typename Head, typename Tail>
    using Cons = typename cons<Head, Tail>::type;

    template <typename T>
    struct alignof_indexed;
    template <typename T, std::size_t I>
    struct alignof_indexed<indexed<T, I>> : std::alignment_of<T> {};
 
    template <typename T>
    struct max_alignment;
    template <typename... T>
    struct max_alignment<std::tuple<T...>> : max<alignof_indexed<T>::value...> {};

    template <std::size_t Align, typename Acc, typename List>
    struct cons_alignment : identity<Acc> {};
    template <std::size_t Align, typename Acc, typename Head, typename... Tail>
    struct cons_alignment<Align, Acc, std::tuple<Head, Tail...>>
    : cons_alignment<
        Align,
        Conditional<Bool<alignof_indexed<Head>::value == Align>, Cons<Head, Acc>, Acc>,
        std::tuple<Tail...>> {};
    template <std::size_t Align, typename Acc, typename List>
    using ConsAlignment = typename cons_alignment<Align, Acc, List>::type;

    template <std::size_t Align, typename Acc, typename List>
    struct sort_impl
    : sort_impl<Align / 2, ConsAlignment<Align, Acc, List>, List> {};
    template <typename Acc, typename List>
    struct sort_impl<0, Acc, List> : identity<Acc> {};

    template <typename List>
    struct sort : sort_impl<max_alignment<List>::value, std::tuple<>, List> {};
} // namespace my

#endif // MY_TUPLE_HPP

