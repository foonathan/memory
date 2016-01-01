// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "detail/free_list_array.hpp"

#include <catch.hpp>

#include "detail/free_list.hpp"
#include "detail/small_free_list.hpp"
#include "static_allocator.hpp"

using namespace foonathan::memory;
using namespace detail;

TEST_CASE("detail::log2_access_policy", "[detail][pool]")
{
    using ap = detail::log2_access_policy;
    REQUIRE(ap::index_from_size(1) == 0u);
    REQUIRE(ap::index_from_size(2) == 1u);
    REQUIRE(ap::index_from_size(3) == 2u);
    REQUIRE(ap::index_from_size(4) == 2u);
    REQUIRE(ap::index_from_size(5) == 3u);
    REQUIRE(ap::index_from_size(6) == 3u);
    REQUIRE(ap::index_from_size(8) == 3u);
    REQUIRE(ap::index_from_size(9) == 4u);

    REQUIRE(ap::size_from_index(0) == 1u);
    REQUIRE(ap::size_from_index(1) == 2u);
    REQUIRE(ap::size_from_index(2) == 4u);
    REQUIRE(ap::size_from_index(3) == 8u);
}

TEST_CASE("detail::free_list_array", "[detail][pool]")
{
    static_allocator_storage<1024> memory;
    detail::fixed_memory_stack stack(&memory);
    SECTION("power of two max size, small list")
    {
        using array = detail::free_list_array<detail::small_free_memory_list,
                                        detail::log2_access_policy>;
        array arr(stack, stack.top() + 1024, 16);
        REQUIRE(arr.max_node_size() == 16u);
        REQUIRE(arr.size() == 5u);

        REQUIRE(arr.get(1u).node_size() == 1u);
        REQUIRE(arr.get(2u).node_size() == 2u);
        REQUIRE(arr.get(3u).node_size() == 4u);
        REQUIRE(arr.get(4u).node_size() == 4u);
        REQUIRE(arr.get(5u).node_size() == 8u);
        REQUIRE(arr.get(9u).node_size() == 16u);
        REQUIRE(arr.get(16u).node_size() == 16u);
    }
    SECTION("non power of two max size, small list")
    {
        using array = detail::free_list_array<detail::small_free_memory_list,
                                        detail::log2_access_policy>;
        array arr(stack, stack.top() + 1024, 15);
        REQUIRE(arr.max_node_size() == 16u);
        REQUIRE(arr.size() == 5u);

        REQUIRE(arr.get(1u).node_size() == 1u);
        REQUIRE(arr.get(2u).node_size() == 2u);
        REQUIRE(arr.get(3u).node_size() == 4u);
        REQUIRE(arr.get(4u).node_size() == 4u);
        REQUIRE(arr.get(5u).node_size() == 8u);
        REQUIRE(arr.get(9u).node_size() == 16u);
        REQUIRE(arr.get(15u).node_size() == 16u);
    }
    SECTION("non power of two max size, normal list")
    {
        using array = detail::free_list_array<detail::free_memory_list,
                                        detail::log2_access_policy>;
        array arr(stack, stack.top() + 1024, 15);
        REQUIRE(arr.max_node_size() == 16u);
        REQUIRE(arr.size() <= 5u);

        REQUIRE(arr.get(1u).node_size() == detail::free_memory_list::min_element_size);
        REQUIRE(arr.get(2u).node_size() == detail::free_memory_list::min_element_size);
        REQUIRE(arr.get(9u).node_size() == 16u);
        REQUIRE(arr.get(15u).node_size() == 16u);
    }
}
