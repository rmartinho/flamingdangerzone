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

    template <bool B>
    using Bool = std::integral_constant<bool, B>;

    template <typename Cond, typename Then, typename Else>
    using Conditional = typename std::conditional<Cond::value, Then, Else>::type;

    template <typename... T>
    struct All : Bool<true> {};
    template <typename Head, typename... Tail>
    struct All<Head, Tail...> : Conditional<Head, All<Tail...>, Bool<false>> {};

    enum class enabler {};
    template <typename... Cond>
    using EnableIf = typename std::enable_if<All<Cond...>::value, enabler>::type;

    // indices
    template <std::size_t... I>
    struct indices {};
    template <std::size_t I>
    using index = std::integral_constant<std::size_t, I>;
    // wait a moment! GCC needs a goat sacrifice
} // namespace my
namespace std {
    template < ::std::size_t I, ::std::size_t... Is>
    struct tuple_element<I, ::my::indices<Is...>>
    : ::std::tuple_element<I, ::std::tuple< ::my::index<Is>...>> {};
} // namespace std
namespace my {
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

    template <typename Tuple, std::size_t... I>
    using ShuffleTuple = std::tuple<TupleElement<I, Tuple>...>;
    
    template <std::size_t... I, typename Tuple>
    ShuffleTuple<Tuple, I...> forward_shuffled_tuple(indices<I...>, Tuple&& t) {
        return std::forward_as_tuple(std::get<I>(std::forward<Tuple>(t))...);
    }
    template <std::size_t... I, typename... T>
    ShuffleTuple<std::tuple<T&&...>, I...> forward_shuffled(indices<I...> map, T&&... t) {
        return forward_shuffled_tuple(map, std::forward_as_tuple(std::forward<T>(t)...));
    }

    // GCC needs another goat sacrifice
    template <typename Ts, typename Us>
    struct pairwise_convertible : Bool<false> {};
    template <>
    struct pairwise_convertible<std::tuple<>, std::tuple<>> : Bool<true> {};
    template <typename THead, typename... TTail, typename UHead, typename... UTail>
    struct pairwise_convertible<std::tuple<THead, TTail...>, std::tuple<UHead, UTail...>>
    : All<std::is_convertible<THead, UHead>, pairwise_convertible<std::tuple<TTail...>, std::tuple<UTail...>>> {};

    template <typename From, typename To>
    struct convert_layout_map;

    template <typename From, std::size_t... To>
    struct convert_layout_map<From, indices<To...>>
    : identity<std::tuple<TupleElement<To, From>...>> {};

    template <typename From, typename To>
    using ConvertLayoutMap = typename convert_layout_map<From, To>::type;

    template <std::size_t I, typename... T>
    using PackElement = TupleElement<I, std::tuple<T...>>;

    template <typename... T>
    struct tuple : private OptimalStorage<T...> {
    private:
        using storage_type = OptimalStorage<T...>;
        using to_interface = MapToInterface<T...>;
        using to_storage = MapToStorage<T...>;
        template <typename... U>
        using MapFor = ConvertLayoutMap<typename tuple<U...>::to_interface, to_interface>;

    public:
        constexpr tuple() = default;

        explicit tuple(T const&... t)
        : storage_type { forward_shuffled(to_interface{}, t...) } {
            static_assert(All<std::is_copy_constructible<T>...>::value,
                "all elements must be copy constructible");
        }
        template <typename... U,
                  EnableIf<pairwise_convertible<std::tuple<U...>, std::tuple<T...>>>...>
        explicit tuple(U&&... u)
        : storage_type { forward_shuffled(to_interface{}, std::forward<U>(u)...) } {
            static_assert(sizeof...(T) == sizeof...(U),
                "number of arguments must match tuple size");
        }

        tuple(tuple const&) = default;
        tuple(tuple&&) = default;

        template <typename... U,
                  EnableIf<std::is_constructible<T, U const&>...>...>
        constexpr tuple(tuple<U...> const& t)
        : storage_type { forward_shuffled_tuple(MapFor<U...>{}, t) } {
            static_assert(sizeof...(T) == sizeof...(U),
                "source tuple size must match destination tuple size");
        }
        template <typename... U,
                  EnableIf<std::is_constructible<T, U&&>...>...>
        constexpr tuple(tuple<U...>&& t)
        : storage_type { forward_shuffled_tuple(MapFor<U...>{}, std::move(t)) } {
            static_assert(sizeof...(T) == sizeof...(U),
                "source tuple size must match destination tuple size");
        }

        template <typename U1, typename U2,
                  EnableIf<std::is_convertible<U1 const&, PackElement<0, T...>>,
                           std::is_convertible<U2 const&, PackElement<1, T...>>>...>
        constexpr tuple(std::pair<U1, U2> const& p)
        : tuple { p.first, p.second } {
            static_assert(sizeof...(T) == 2,
                "tuple size must be 2");
        }
        template <typename U1, typename U2,
                  EnableIf<std::is_convertible<U1 const&, PackElement<0, T...>>,
                           std::is_convertible<U2 const&, PackElement<1, T...>>>...>
        constexpr tuple(std::pair<U1, U2>&& p)
        : tuple { std::move(p.first), std::move(p.second) } {
            static_assert(sizeof...(T) == 2,
                "tuple size must be 2");
        }

        template <typename... U,
                  EnableIf<std::is_constructible<T, U const&>...>...>
        constexpr tuple(std::tuple<U...> const& t)
        : tuple { forward_shuffled_tuple(to_interface{}, t) } {
            static_assert(sizeof...(T) == sizeof...(U),
                "source tuple size must match destination tuple size");
        }
        template <typename... U,
                  EnableIf<std::is_constructible<T, U&&>...>...>
        constexpr tuple(std::tuple<U...>&& t)
        : tuple { forward_shuffled_tuple(to_interface{}, std::move(t)) } {
            static_assert(sizeof...(T) == sizeof...(U),
                "source tuple size must match destination tuple size");
        }

        tuple& operator=(tuple const&) = default;
        tuple& operator=(tuple&&) = default;

        template <typename... U>
        tuple& operator=(tuple<U...> const& t) {
            static_assert(sizeof...(T) == sizeof...(U),
                "tuples can only be assigned to tuples with the same size");
            static_assert(All<std::is_assignable<T&, U const&>...>::value,
                "all elements must be assignable to the corresponding element");
            storage_type::operator=(forward_shuffled_tuple(MapFor<U...>{}, t));
            return *this;
        }
        template <typename... U>
        tuple& operator=(tuple<U...>&& t) {
            static_assert(sizeof...(T) == sizeof...(U),
                "tuples can only be assigned to tuples with the same size");
            static_assert(All<std::is_assignable<T&, U&&>...>::value,
                "all elements must be move-assignable to the corresponding element");
            storage_type::operator=(forward_shuffled_tuple(MapFor<U...>{}, std::move(t)));
            return *this;
        }

        template <typename U1, typename U2>
        tuple& operator=(std::pair<U1, U2> const& p) {
            static_assert(sizeof...(T) == 2,
                "tuple size must be 2");
            static_assert(std::is_assignable<PackElement<0, T...>&, U1 const&>::value,
                "first pair element must be assignable to first tuple element");
            static_assert(std::is_assignable<PackElement<1, T...>&, U2 const&>::value,
                "second pair element must be assignable to second tuple element");
            storage_type::operator=(forward_shuffled(to_interface{}, p.first, p.second));
            return *this;
        }
        template <typename U1, typename U2>
        tuple& operator=(std::pair<U1, U2>&& p) {
            static_assert(sizeof...(T) == 2,
                "tuple size must be 2");
            static_assert(std::is_assignable<PackElement<0, T...>&, U1&&>::value,
                "first pair element must be move-assignable to first tuple element");
            static_assert(std::is_assignable<PackElement<1, T...>&, U2&&>::value,
                "second pair element must be move-assignable to second tuple element");
            storage_type::operator=(forward_shuffled(to_interface{}, std::move(p.first), std::move(p.second)));
            return *this;
        }

        template <typename... U>
        tuple& operator=(std::tuple<U...> const& t) {
            static_assert(sizeof...(T) == sizeof...(U),
                "tuples can only be assigned to tuples with the same size");
            static_assert(All<std::is_assignable<T&, U const&>...>::value,
                "all elements must be assignable to the corresponding element");
            storage_type::operator=(forward_shuffled_tuple(to_interface{}, t));
            return *this;
        }
        template <typename... U>
        tuple& operator=(std::tuple<U...>&& t) {
            static_assert(sizeof...(T) == sizeof...(U),
                "tuples can only be assigned to tuples with the same size");
            static_assert(All<std::is_assignable<T&, U&&>...>::value,
                "all elements must be move-assignable to the corresponding element");
            storage_type::operator=(forward_shuffled_tuple(to_interface{}, std::move(t)));
            return *this;
        }

        void swap(tuple& t)
        noexcept(noexcept(std::declval<storage_type>().swap(std::declval<storage_type>()))) {
            storage_type::swap(t);
        }

        template <std::size_t I, typename... U>
        friend TupleElement<I, std::tuple<U...>>& get(tuple<U...>& t);
        template <std::size_t I, typename... U>
        friend TupleElement<I, std::tuple<U...>>&& get(tuple<U...>&& t);
        template <std::size_t I, typename... U>
        friend TupleElement<I, std::tuple<U...>> const& get(tuple<U...> const& t);
        template <typename... U>
        friend class tuple;
    };

    template <std::size_t I, typename... U>
    TupleElement<I, std::tuple<U...>>& get(tuple<U...>& t) {
        return std::get<TupleElement<I, MapToStorage<U...>>::value>(t);
    }
    template <std::size_t I, typename... U>
    TupleElement<I, std::tuple<U...>>&& get(tuple<U...>&& t) {
        return std::get<TupleElement<I, MapToStorage<U...>>::value>(t);
    }
    template <std::size_t I, typename... U>
    TupleElement<I, std::tuple<U...>> const& get(tuple<U...> const& t) {
        return std::get<TupleElement<I, MapToStorage<U...>>::value>(t);
    }
} // namespace my

#endif // MY_TUPLE_HPP

