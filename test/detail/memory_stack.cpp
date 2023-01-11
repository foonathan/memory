// Copyright (C) 2015-2023 Jonathan Müller and foonathan/memory contributors
// SPDX-License-Identifier: Zlib

#include "detail/memory_stack.hpp"

#include <doctest/doctest.h>

#include "detail/align.hpp"
#include "detail/debug_helpers.hpp"
#include "detail/utility.hpp"
#include "static_allocator.hpp"

using namespace foonathan::memory;
using namespace detail;

TEST_CASE("detail::fixed_memory_stack")
{
    fixed_memory_stack stack;
    REQUIRE(stack.top() == nullptr);

    SUBCASE("allocate")
    {
        static_allocator_storage<1024> memory;
        stack    = fixed_memory_stack{&memory};
        auto end = stack.top() + 1024;

        REQUIRE(stack.top() == reinterpret_cast<char*>(&memory));

        SUBCASE("alignment for allocate")
        {
            auto ptr = stack.allocate(end, 13, 1u);
            REQUIRE(ptr);
            REQUIRE(is_aligned(ptr, 1u));

            ptr = stack.allocate(end, 10, 2u);
            REQUIRE(ptr);
            REQUIRE(is_aligned(ptr, 2u));

            ptr = stack.allocate(end, 10, max_alignment);
            REQUIRE(ptr);
            REQUIRE(is_aligned(ptr, max_alignment));

            ptr = stack.allocate(end, 10, 2 * max_alignment);
            REQUIRE(ptr);
            REQUIRE(is_aligned(ptr, 2 * max_alignment));
        }
        SUBCASE("allocate/unwind")
        {
            REQUIRE(stack.allocate(end, 10u, 1u));
            auto diff = std::size_t(stack.top() - reinterpret_cast<char*>(&memory));
            REQUIRE(diff == 2 * debug_fence_size + 10u);

            REQUIRE(stack.allocate(end, 16u, 1u));
            auto diff2 = std::size_t(stack.top() - reinterpret_cast<char*>(&memory));
            REQUIRE(diff2 == 2 * debug_fence_size + 16u + diff);

            stack.unwind(reinterpret_cast<char*>(&memory) + diff);
            REQUIRE(stack.top() == reinterpret_cast<char*>(&memory) + diff);

            auto top = stack.top();
            REQUIRE(!stack.allocate(end, 1024, 1));
            REQUIRE(stack.top() == top);
        }
    }
    SUBCASE("move")
    {
        static_allocator_storage<1024> memory;
        auto                           end = reinterpret_cast<char*>(&memory) + 1024;

        fixed_memory_stack other(reinterpret_cast<char*>(&memory));
        REQUIRE(other.top() == reinterpret_cast<char*>(&memory));

        stack = detail::move(other);
        REQUIRE(stack.top() == reinterpret_cast<char*>(&memory));

        REQUIRE(!other.allocate(end, 10, 1));
        REQUIRE(stack.allocate(end, 10, 1));
        auto top = stack.top();

        other = detail::move(stack);
        REQUIRE(other.top() == top);
        REQUIRE(!stack.allocate(end, 10, 1));
        REQUIRE(other.allocate(end, 10, 1));
    }
}
