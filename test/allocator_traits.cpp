// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "allocator_traits.hpp"

#include <catch.hpp>

#include <type_traits>
#include <memory/allocator_traits.hpp>

using namespace foonathan::memory;

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

template <typename T, typename ... Dummy>
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
    struct empty_raw {};
    test_type_statefulness<empty_raw, empty_raw, false>();

    struct non_empty_raw {int i;};
    test_type_statefulness<non_empty_raw, non_empty_raw, true>();

    struct explicit_stateful_raw
    {
        using is_stateful = std::true_type;
    };
    test_type_statefulness<explicit_stateful_raw, explicit_stateful_raw, true>();

    struct explicit_stateless_raw
    {
        using is_stateful = std::false_type;
        int i;
    };
    test_type_statefulness<explicit_stateless_raw, explicit_stateless_raw, false>();

    test_type_statefulness<standard_alloc<char>, standard_alloc<char>, false>();
    test_type_statefulness<standard_alloc<int>, standard_alloc<char>, false>();

    test_type_statefulness<standard_multi_arg<char, int, int>, standard_multi_arg<char, int, int>, false>();
    test_type_statefulness<standard_multi_arg<int, int, int>, standard_multi_arg<char, int, int>, false>();

    test_type_statefulness<standard_with_rebind<char, char>, standard_with_rebind<char, int>, false>();
    test_type_statefulness<standard_with_rebind<int, char>, standard_with_rebind<char, int>, false>();
}

template <class Allocator>
void test_node(Allocator &alloc)
{
    auto ptr = allocator_traits<Allocator>::allocate_node(alloc, 1, 1);
    allocator_traits<Allocator>::deallocate_node(alloc, ptr, 1, 1);
}

template <class Allocator>
void test_array(Allocator &alloc)
{
    auto ptr = allocator_traits<Allocator>::allocate_array(alloc, 1, 1, 1);
    allocator_traits<Allocator>::deallocate_array(alloc, ptr, 1, 1, 1);
}

template <class Allocator>
void test_max_getter(const Allocator &alloc, std::size_t alignment, std::size_t node, std::size_t array)
{
    auto i = allocator_traits<Allocator>::max_alignment(alloc);
    REQUIRE(i == alignment);
    i = allocator_traits<Allocator>::max_node_size(alloc);
    REQUIRE(i == node);
    i = allocator_traits<Allocator>::max_array_size(alloc);
    REQUIRE(i == array);
}

TEST_CASE("allocator_traits", "[core]")
{
    struct min_raw_allocator
    {
        bool alloc_node = false, dealloc_node = false;

        void* allocate_node(std::size_t, std::size_t)
        {
            alloc_node = true;
            return nullptr;
        }

        void deallocate_node(void*, std::size_t, std::size_t) FOONATHAN_NOEXCEPT
        {
            dealloc_node = true;
        }
    };

    struct standard_allocator
    {
        using value_type = char;

        bool alloc = false, dealloc = false;

        char* allocate(std::size_t)
        {
            alloc = true;
            return nullptr;
        }

        void deallocate(char*, std::size_t) FOONATHAN_NOEXCEPT
        {
            dealloc = true;
        }
    };

    SECTION("node")
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
        {};

        // raw is preferred over standard
        both_alloc both;
        test_node(both);
        REQUIRE(both.alloc_node);
        REQUIRE(both.dealloc_node);
        REQUIRE(!both.alloc);
        REQUIRE(!both.dealloc);
    }
    SECTION("array")
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

            void deallocate_array(void*, std::size_t, std::size_t, std::size_t) FOONATHAN_NOEXCEPT
            {
                dealloc_array = true;
            }
        };

        // array works
        array_raw array;
        test_array(array);
        REQUIRE(array.alloc_array);
        REQUIRE(array.dealloc_array);

        struct array_node : min_raw_allocator, array_raw {};
        // array works over node
        array_node array2;
        test_array(array2);
        REQUIRE(array2.alloc_array);
        REQUIRE(array2.dealloc_array);
        REQUIRE(!array2.alloc_node);
        REQUIRE(!array2.dealloc_node);

        struct array_std : standard_allocator, array_raw {};
        // array works over standard
        array_std array3;
        test_array(array3);
        REQUIRE(array3.alloc_array);
        REQUIRE(array3.dealloc_array);
        REQUIRE(!array3.alloc);
        REQUIRE(!array3.dealloc);

        struct array_node_std : standard_allocator, array_raw, min_raw_allocator {};
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
    SECTION("max getter")
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

        struct with_node_array : with_node, with_array {};
        with_node_array node_array;
        test_max_getter(node_array, detail::max_alignment, 1, 2);

        struct with_everything : with_node_array, with_alignment {};
        with_everything everything;
        test_max_getter(everything, detail::max_alignment * 2, 1, 2);
    }
}
