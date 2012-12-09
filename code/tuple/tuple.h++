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

    // indices
    template <std::size_t... I>
    struct indices {};
    template <std::size_t I>
    using index = std::integral_constant<std::size_t, I>;
    /* GCC bug here
    template <std::size_t... I>
    using indices = std::tuple<index<I>...>;
    */

    template <std::size_t... N>
    struct indices_builder {
        using type = indices<N...>;
        using next = indices_builder<N..., sizeof...(N)>;
    };
    template <std::size_t N>
    struct indices_up_to {
        using builder = typename indices_up_to<N-1>::builder::next;
        using type = typename builder::type;
    };
    template <>
    struct indices_up_to<0> {
        using builder = indices_builder<>;
        using type = std::tuple<>;
    };
    template <typename Tuple>
    using IndicesFor = typename indices_up_to<std::tuple_size<Tuple>::value>::type;

    // code starts here
    // wrapper to get alignment of references
    template <typename T>
    struct member { T _; };

    // packing types with indices
    template <typename T, std::size_t I>
    struct indexed {
        using type = T;
        static constexpr auto i = I;
    };

    // attaching index information
    template <typename Acc, typename... T>
    struct with_indices_impl : identity<Acc> {};
    template <typename... Acc, typename Head, typename... Tail>
    struct with_indices_impl<std::tuple<Acc...>, Head, Tail...>
    : with_indices_impl<std::tuple<Acc..., indexed<Head, sizeof...(Acc)>>, Tail...> {};

    template <typename List>
    struct with_indices;
    template <typename... T>
    struct with_indices<std::tuple<T...>> : with_indices_impl<std::tuple<>, T...> {};
    template <typename List>
    using WithIndices = typename with_indices<List>::type;

    // building lists by appending at the appropriate end
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

    // compute alignments
    template <typename T>
    struct alignof_indexed;
    template <typename T, std::size_t I>
    struct alignof_indexed<indexed<T, I>> : std::alignment_of<member<T>> {};
 
    // find maximum alignment
    template <typename List>
    struct max_alignment;
    template <typename... T>
    struct max_alignment<std::tuple<T...>> : max<alignof_indexed<T>::value...> {};

    // Cons all the types with a given alignment into an accumulator
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

    // Sort by iterating down from the maximum alignment
    template <std::size_t Align, typename Acc, typename List>
    struct sort_impl
    : sort_impl<Align / 2, ConsAlignment<Align, Acc, List>, List> {};
    template <typename Acc, typename List>
    struct sort_impl<0, Acc, List> : identity<Acc> {};

    template <typename List>
    struct sort : sort_impl<max_alignment<List>::value, std::tuple<>, List> {};
    template <typename List>
    using Sort = typename sort<List>::type;

    // Splitting the type and index information
    template <typename List>
    struct split;
    template <typename... T, std::size_t... I>
    struct split<std::tuple<indexed<T, I>...>> {
        using tuple = std::tuple<T...>;
        using map = indices<I...>;
    };

    // Reversing the map
    template <typename List>
    struct inherit_all;
    template <typename... T>
    struct inherit_all<std::tuple<T...>> : T... {};

    template <typename T, std::size_t I, std::size_t J>
    using indexed2 = indexed<indexed<T, I>, J>;

    template <std::size_t Target, std::size_t Result, typename T>
    index<Result> find_index_impl(indexed2<T, Target, Result> const&);

    template <std::size_t N, typename List>
    using find_index = decltype(find_index_impl<N>(inherit_all<List>{}));

    template <typename List, typename Indices = IndicesFor<List>>
    struct map_to_storage {};
    template <typename List, std::size_t... I>
    struct map_to_storage<List, indices<I...>>
    : identity<indices<find_index<I, List>::value...>> {};

    // All the optimal layout info
    template <typename List>
    struct optimal_order {
        using sorted = Sort<WithIndices<List>>;
        using tuple = typename split<sorted>::tuple;
        using to_interface = typename split<sorted>::map;
        using to_storage = typename map_to_storage<WithIndices<sorted>>::type;
    };

    template <typename... T>
    using OptimalStorage = typename optimal_order<std::tuple<T...>>::tuple;
    template <typename... T>
    using MapToInterface = typename optimal_order<std::tuple<T...>>::to_interface;
    template <typename... T>
    using MapToStorage = typename optimal_order<std::tuple<T...>>::to_storage;
} // namespace my

#endif // MY_TUPLE_HPP

