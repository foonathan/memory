// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "memory_arena.hpp"

#include <catch.hpp>

#include "static_allocator.hpp"

using namespace foonathan::memory;
using namespace detail;

TEST_CASE("detail::memory_block_stack", "[detail][arena]")
{
    memory_block_stack stack;
    REQUIRE(stack.empty());

    static_allocator_storage<1024> memory;

    stack.push({&memory, 1024});
    REQUIRE(!stack.empty());

    auto top = stack.top();
    REQUIRE(top.memory >= static_cast<void*>(&memory));
    REQUIRE(top.size <= 1024);
    REQUIRE(is_aligned(top.memory, max_alignment));

    SECTION("pop")
    {
        auto block = stack.pop();
        REQUIRE(block.size == 1024);
        REQUIRE(block.memory == static_cast<void*>(&memory));
    }
    SECTION("steal_top")
    {
        memory_block_stack other;

        other.steal_top(stack);
        REQUIRE(stack.empty());
        REQUIRE(!other.empty());

        auto top = other.top();
        REQUIRE(top.memory >= static_cast<void*>(&memory));
        REQUIRE(top.size <= 1024);
        REQUIRE(is_aligned(top.memory, max_alignment));
    }

    static_allocator_storage<1024> a, b, c;
    stack.push({&a, 1024});
    stack.push({&b, 1024});
    stack.push({&c, 1024});

    SECTION("multiple pop")
    {
        auto block = stack.pop();
        REQUIRE(block.memory == static_cast<void*>(&c));
        block = stack.pop();
        REQUIRE(block.memory == static_cast<void*>(&b));
        block = stack.pop();
        REQUIRE(block.memory == static_cast<void*>(&a));
        block = stack.pop();
        REQUIRE(block.memory == static_cast<void*>(&memory));
    }
    SECTION("multiple steal_from")
    {
        memory_block_stack other;

        other.steal_top(stack);
        other.steal_top(stack);
        other.steal_top(stack);
        other.steal_top(stack);

        REQUIRE(stack.empty());

        auto block = other.pop();
        REQUIRE(block.memory == static_cast<void*>(&memory));
        block = other.pop();
        REQUIRE(block.memory == static_cast<void*>(&a));
        block = other.pop();
        REQUIRE(block.memory == static_cast<void*>(&b));
        block = other.pop();
        REQUIRE(block.memory == static_cast<void*>(&c));
    }
    SECTION("move")
    {
        memory_block_stack other = detail::move(stack);
        REQUIRE(stack.empty());
        REQUIRE(!other.empty());

        auto block = other.pop();
        REQUIRE(block.memory == static_cast<void*>(&c));

        stack = detail::move(other);
        REQUIRE(other.empty());
        REQUIRE(!stack.empty());

        block = stack.pop();
        REQUIRE(block.memory == static_cast<void*>(&b));
    }
}
