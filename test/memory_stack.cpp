// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "memory_stack.hpp"

#include <catch.hpp>

#include "allocator_storage.hpp"
#include "test_allocator.hpp"

using namespace foonathan::memory;

TEST_CASE("memory_stack", "[stack]")
{
    test_allocator alloc;
    memory_stack<allocator_reference<test_allocator>> stack(100, alloc);
    REQUIRE(alloc.no_allocated() == 1u);
    REQUIRE(stack.capacity() <= 100);
    auto capacity = stack.capacity();

    SECTION("empty unwind")
    {
        auto m = stack.top();
        stack.unwind(m);
        REQUIRE(capacity <= 100);
        REQUIRE(alloc.no_allocated() == 1u);
        REQUIRE(alloc.no_deallocated() == 0u);
    }
    SECTION("normal allocation/unwind")
    {
        stack.allocate(10, 1);
        REQUIRE(stack.capacity() == capacity - 10 - 2 * detail::debug_fence_size);

        auto m = stack.top();

        auto memory = stack.allocate(10, 16);
        REQUIRE(detail::align_offset(memory, 16) == 0u);

        stack.unwind(m);
        REQUIRE(stack.capacity() ==
                capacity - 10 - 2 * detail::debug_fence_size);

        REQUIRE(stack.allocate(10, 16) == memory);
        REQUIRE(alloc.no_allocated() == 1u);
        REQUIRE(alloc.no_deallocated() == 0u);
    }
    SECTION("multiple blok allocation/unwind")
    {
        // note: tests are mostly hoping not to get a segfault

        stack.allocate(10, 1);
        auto m = stack.top();

        auto old_next = stack.next_capacity();

        stack.allocate(100, 1);
        REQUIRE(stack.next_capacity() > old_next);
        REQUIRE(alloc.no_allocated() == 2u);
        REQUIRE(alloc.no_deallocated() == 0u);

        auto m2 = stack.top();
        stack.allocate(10, 1);
        stack.unwind(m2);
        stack.allocate(20, 1);

        stack.unwind(m);
        REQUIRE(alloc.no_allocated() == 2u);
        REQUIRE(alloc.no_deallocated() == 0u);

        stack.allocate(10, 1);

        stack.shrink_to_fit();
        REQUIRE(alloc.no_allocated() == 1u);
        REQUIRE(alloc.no_deallocated() == 1u);
    }
}

