// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "detail/memory_stack.hpp"

#include <catch.hpp>

#include "detail/align.hpp"

using namespace foonathan::memory;
using namespace detail;

TEST_CASE("detail::fixed_memory_stack", "[detail][stack]")
{
    fixed_memory_stack stack;
    REQUIRE(stack.top() == nullptr);
    REQUIRE(stack.end() == nullptr);

    SECTION("allocate")
    {
        alignas(max_alignment) char memory[1024];
        stack = {memory, 1024};
        REQUIRE(stack.top() == memory);
        REQUIRE(stack.end() == memory + 1024);

        SECTION("alignment for allocate")
        {
            auto ptr = stack.allocate(13, 1u);
            REQUIRE(ptr);
            REQUIRE(is_aligned(ptr, 1u));

            ptr = stack.allocate(10, 2u);
            REQUIRE(ptr);
            REQUIRE(is_aligned(ptr, 2u));

            ptr = stack.allocate(10, max_alignment);
            REQUIRE(ptr);
            REQUIRE(is_aligned(ptr, max_alignment));

            ptr = stack.allocate(10, 2 * max_alignment);
            REQUIRE(ptr);
            REQUIRE(is_aligned(ptr, 2 * max_alignment));
        }
        SECTION("allocate/unwind")
        {
            REQUIRE(stack.allocate(10u, 1u));
            auto diff = std::size_t(stack.top() - memory);
            REQUIRE(diff == 2 * debug_fence_size + 10u);

            REQUIRE(stack.allocate(16u, 1u));
            auto diff2 = std::size_t(stack.top() - memory);
            REQUIRE(diff2 == 2 * debug_fence_size + 16u + diff);

            stack.unwind(memory + diff);
            REQUIRE(stack.top() == memory + diff);
            REQUIRE(stack.end() == memory + 1024);

            auto top = stack.top();
            REQUIRE(!stack.allocate(1024, 1));
            REQUIRE(stack.top() == top);
        }
    }
    SECTION("move")
    {
        alignas(max_alignment) char memory[1024];

        fixed_memory_stack other(memory, memory + 1024);
        REQUIRE(other.top() == memory);
        REQUIRE(other.end() == memory + 1024);

        stack = detail::move(other);
        REQUIRE(stack.top() == memory);
        REQUIRE(stack.end() == memory + 1024);

        REQUIRE(!other.allocate(10, 1));
        REQUIRE(stack.allocate(10, 1));
        auto top = stack.top();

        other = detail::move(stack);
        REQUIRE(other.top() == top);
        REQUIRE(!stack.allocate(10, 1));
        REQUIRE(other.allocate(10, 1));
    }
}
