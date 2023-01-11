// Copyright (C) 2015-2023 Jonathan Müller and foonathan/memory contributors
// SPDX-License-Identifier: Zlib

#include "allocator_traits.hpp"

#include <doctest/doctest.h>

#include <type_traits>

#include "heap_allocator.hpp"

using namespace foonathan::memory;

static_assert(is_raw_allocator<std::allocator<char>>::value, "");
static_assert(allocator_is_raw_allocator<std::allocator<char>>::value, "");
static_assert(is_raw_allocator<std::allocator<int>>::value, "");
static_assert(allocator_is_raw_allocator<std::allocator<int>>::value, "");
static_assert(is_raw_allocator<heap_allocator>::value, "");
static_assert(!is_raw_allocator<int>::value, "");

template <typename T>
struct std_allocator_construct
{
    using value_type = T;
    using pointer    = T;

    void construct(pointer, T) {}
};

static_assert(!allocator_is_raw_allocator<std_allocator_construct<int>>::value, "");
static_assert(!is_raw_allocator<std_allocator_construct<int>>::value, "");

struct raw_allocator_specialized
{
};

namespace foonathan
{
    namespace memory
    {
        template <>
        struct allocator_traits<raw_allocator_specialized>
        {
        };
    } // namespace memory
} // namespace foonathan

static_assert(is_raw_allocator<raw_allocator_specialized>::value, "");

template <class Allocator, class Type, bool Stateful>
void test_type_statefulness()
{
    using type = typename allocator_traits<Allocator>::allocator_type;
    static_assert(std::is_same<type, Type>::value, "allocator_type failed");
    using stateful = typename allocator_traits<Allocator>::is_stateful;
    static_assert(stateful::value == Stateful, "is_stateful failed");
}

template <typename T>
struct standard_alloc
{
    using value_type = T;
};

template <typename T, typename... Dummy>
struct standard_multi_arg
{
    using value_type = T;
};

template <typename T, typename Dummy>
struct standard_with_rebind
{
    using value_type = T;

    template <typename U>
    struct rebind
    {
        using other = standard_with_rebind<U, int>;
    };
};

void instantiate_test_type_statefulness()
{
    struct empty_raw
    {
    };
    test_type_statefulness<empty_raw, empty_raw, false>();

    struct non_empty_raw
    {
        int i;
    };
    test_type_statefulness<non_empty_raw, non_empty_raw, true>();

    struct explicit_stateful_raw
    {
        using is_stateful = std::true_type;
    };
    (void)explicit_stateful_raw::is_stateful();
    test_type_statefulness<explicit_stateful_raw, explicit_stateful_raw, true>();

    struct explicit_stateless_raw
    {
        using is_stateful = std::false_type;
        int i;
    };
    (void)explicit_stateless_raw::is_stateful();
    test_type_statefulness<explicit_stateless_raw, explicit_stateless_raw, false>();

    test_type_statefulness<standard_alloc<char>, standard_alloc<char>, false>();
    test_type_statefulness<standard_alloc<int>, standard_alloc<char>, false>();

    test_type_statefulness<standard_multi_arg<char, int, int>, standard_multi_arg<char, int, int>,
                           false>();
    test_type_statefulness<standard_multi_arg<int, int, int>, standard_multi_arg<char, int, int>,
                           false>();

    test_type_statefulness<standard_with_rebind<char, char>, standard_with_rebind<char, int>,
                           false>();
    test_type_statefulness<standard_with_rebind<int, char>, standard_with_rebind<char, int>,
                           false>();
}

template <class Allocator>
void test_node(Allocator& alloc)
{
    auto ptr = allocator_traits<Allocator>::allocate_node(alloc, 1, 1);
    allocator_traits<Allocator>::deallocate_node(alloc, ptr, 1, 1);
}

template <class Allocator>
void test_array(Allocator& alloc)
{
    auto ptr = allocator_traits<Allocator>::allocate_array(alloc, 1, 1, 1);
    allocator_traits<Allocator>::deallocate_array(alloc, ptr, 1, 1, 1);
}

template <class Allocator>
void test_max_getter(const Allocator& alloc, std::size_t alignment, std::size_t node,
                     std::size_t array)
{
    auto i = allocator_traits<Allocator>::max_alignment(alloc);
    REQUIRE(i == alignment);
    i = allocator_traits<Allocator>::max_node_size(alloc);
    REQUIRE(i == node);
    i = allocator_traits<Allocator>::max_array_size(alloc);
    REQUIRE(i == array);
}

TEST_CASE("allocator_traits")
{
    struct min_raw_allocator
    {
        bool alloc_node = false, dealloc_node = false;

        void* allocate_node(std::size_t, std::size_t)
        {
            alloc_node = true;
            return nullptr;
        }

        void deallocate_node(void*, std::size_t, std::size_t) noexcept
        {
            dealloc_node = true;
        }
    };

    static_assert(is_raw_allocator<min_raw_allocator>::value, "");

    struct standard_allocator
    {
        using value_type = char;

        bool alloc = false, dealloc = false;

        char* allocate(std::size_t)
        {
            alloc = true;
            return nullptr;
        }

        void deallocate(char*, std::size_t) noexcept
        {
            dealloc = true;
        }
    };

    static_assert(is_raw_allocator<standard_allocator>::value, "");

    SUBCASE("node")
    {
        // minimum interface works
        min_raw_allocator min;
        test_node(min);
        REQUIRE(min.alloc_node);
        REQUIRE(min.dealloc_node);

        // standard interface works
        standard_allocator std;
        test_node(std);
        REQUIRE(std.alloc);
        REQUIRE(std.dealloc);

        struct both_alloc : min_raw_allocator, standard_allocator
        {
        };

        static_assert(is_raw_allocator<both_alloc>::value, "");

        // raw is preferred over standard
        both_alloc both;
        test_node(both);
        REQUIRE(both.alloc_node);
        REQUIRE(both.dealloc_node);
        REQUIRE(!both.alloc);
        REQUIRE(!both.dealloc);
    }
    SUBCASE("array")
    {
        // minimum interface works
        min_raw_allocator min;
        test_array(min);
        REQUIRE(min.alloc_node);
        REQUIRE(min.dealloc_node);

        // standard interface works
        standard_allocator std;
        test_array(std);
        REQUIRE(std.alloc);
        REQUIRE(std.dealloc);

        struct array_raw
        {
            bool alloc_array = false, dealloc_array = false;

            void* allocate_array(std::size_t, std::size_t, std::size_t)
            {
                alloc_array = true;
                return nullptr;
            }

            void deallocate_array(void*, std::size_t, std::size_t, std::size_t) noexcept
            {
                dealloc_array = true;
            }
        };

        // array works
        array_raw array;
        test_array(array);
        REQUIRE(array.alloc_array);
        REQUIRE(array.dealloc_array);

        struct array_node : min_raw_allocator, array_raw
        {
        };
        static_assert(is_raw_allocator<array_node>::value, "");

        // array works over node
        array_node array2;
        test_array(array2);
        REQUIRE(array2.alloc_array);
        REQUIRE(array2.dealloc_array);
        REQUIRE(!array2.alloc_node);
        REQUIRE(!array2.dealloc_node);

        struct array_std : standard_allocator, array_raw
        {
        };
        // array works over standard
        array_std array3;
        test_array(array3);
        REQUIRE(array3.alloc_array);
        REQUIRE(array3.dealloc_array);
        REQUIRE(!array3.alloc);
        REQUIRE(!array3.dealloc);

        struct array_node_std : standard_allocator, array_raw, min_raw_allocator
        {
        };
        // array works over everything
        array_node_std array4;
        test_array(array4);
        REQUIRE(array4.alloc_array);
        REQUIRE(array4.dealloc_array);
        REQUIRE(!array4.alloc_node);
        REQUIRE(!array4.dealloc_node);
        REQUIRE(!array4.alloc);
        REQUIRE(!array4.dealloc);
    }
    SUBCASE("max getter")
    {
        min_raw_allocator min;
        test_max_getter(min, detail::max_alignment, std::size_t(-1), std::size_t(-1));

        struct with_alignment
        {
            std::size_t max_alignment() const
            {
                return detail::max_alignment * 2;
            }
        };
        with_alignment alignment;
        test_max_getter(alignment, detail::max_alignment * 2, std::size_t(-1), std::size_t(-1));

        struct with_node
        {
            std::size_t max_node_size() const
            {
                return 1;
            }
        };
        with_node node;
        test_max_getter(node, detail::max_alignment, 1, 1);

        struct with_array
        {
            std::size_t max_array_size() const
            {
                return 2;
            }
        };
        with_array array;
        test_max_getter(array, detail::max_alignment, std::size_t(-1), 2);

        struct with_node_array : with_node, with_array
        {
        };
        with_node_array node_array;
        test_max_getter(node_array, detail::max_alignment, 1, 2);

        struct with_everything : with_node_array, with_alignment
        {
        };
        with_everything everything;
        test_max_getter(everything, detail::max_alignment * 2, 1, 2);
    }
}

template <class Allocator>
void test_try_node(Allocator& alloc)
{
    auto ptr = composable_allocator_traits<Allocator>::try_allocate_node(alloc, 1, 1);
    composable_allocator_traits<Allocator>::try_deallocate_node(alloc, ptr, 1, 1);
}

template <class Allocator>
void test_try_array(Allocator& alloc)
{
    auto ptr = composable_allocator_traits<Allocator>::try_allocate_array(alloc, 1, 1, 1);
    composable_allocator_traits<Allocator>::try_deallocate_array(alloc, ptr, 1, 1, 1);
}

TEST_CASE("composable_allocator_traits")
{
    struct min_composable_allocator
    {
        bool alloc_node = false, dealloc_node = false;

        void* allocate_node(std::size_t, std::size_t)
        {
            return nullptr;
        }

        void deallocate_node(void*, std::size_t, std::size_t) noexcept {}

        void* try_allocate_node(std::size_t, std::size_t)
        {
            alloc_node = true;
            return nullptr;
        }

        bool try_deallocate_node(void*, std::size_t, std::size_t) noexcept
        {
            dealloc_node = true;
            return true;
        }
    };
    static_assert(is_composable_allocator<min_composable_allocator>::value, "");

    SUBCASE("node")
    {
        min_composable_allocator alloc;
        test_try_node(alloc);
        REQUIRE(alloc.alloc_node);
        REQUIRE(alloc.dealloc_node);
    }
    SUBCASE("array")
    {
        min_composable_allocator min;
        test_try_array(min);
        REQUIRE(min.alloc_node);
        REQUIRE(min.dealloc_node);

        struct array_composable : min_composable_allocator
        {
            bool alloc_array = false, dealloc_array = false;

            void* try_allocate_array(std::size_t, std::size_t, std::size_t)
            {
                alloc_array = true;
                return nullptr;
            }

            bool try_deallocate_array(void*, std::size_t, std::size_t, std::size_t) noexcept
            {
                dealloc_array = true;
                return true;
            }
        } array;

        test_try_array(array);
        REQUIRE(array.alloc_array);
        REQUIRE(array.dealloc_array);
        REQUIRE(!array.alloc_node);
        REQUIRE(!array.dealloc_node);
    }
}
