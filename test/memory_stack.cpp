// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "memory_stack.hpp"

#include <catch.hpp>

#include "allocator_storage.hpp"
#include "test_allocator.hpp"

using namespace foonathan::memory;

TEST_CASE("memory_stack", "[stack]")
{
    test_allocator                                    alloc;
    memory_stack<allocator_reference<test_allocator>> stack(100, alloc);
    REQUIRE(alloc.no_allocated() == 1u);
    REQUIRE(stack.capacity_left() <= 100);
    auto capacity = stack.capacity_left();

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
        REQUIRE(stack.capacity_left() == capacity - 10 - 2 * detail::debug_fence_size);

        auto m = stack.top();

        auto memory = stack.allocate(10, 16);
        REQUIRE(detail::is_aligned(memory, 16));

        stack.unwind(m);
        REQUIRE(stack.capacity_left() == capacity - 10 - 2 * detail::debug_fence_size);

        REQUIRE(stack.allocate(10, 16) == memory);
        REQUIRE(alloc.no_allocated() == 1u);
        REQUIRE(alloc.no_deallocated() == 0u);
    }
    SECTION("multiple block allocation/unwind")
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
        REQUIRE(m < m2);
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
    SECTION("move")
    {
        auto other = detail::move(stack);
        auto m     = other.top();
        other.allocate(10, 1);
        REQUIRE(alloc.no_allocated() == 1u);

        stack.allocate(10, 1);
        REQUIRE(alloc.no_allocated() == 2u);

        stack = detail::move(other);
        REQUIRE(alloc.no_allocated() == 1u);
        stack.unwind(m);
    }
    SECTION("marker comparision")
    {
        auto m1 = stack.top();
        auto m2 = stack.top();
        REQUIRE(m1 == m2);

        stack.allocate(1, 1);
        auto m3 = stack.top();
        REQUIRE(m1 < m3);

        stack.unwind(m2);
        REQUIRE(stack.top() == m2);
    }
    SECTION("unwinder")
    {
        auto m = stack.top();
        {
            memory_stack_raii_unwind<decltype(stack)> unwind(stack);
            stack.allocate(10, 1);
            REQUIRE(unwind.will_unwind());
            REQUIRE(&unwind.get_stack() == &stack);
            REQUIRE(unwind.get_marker() == m);
        }
        REQUIRE(stack.top() == m);

        memory_stack_raii_unwind<decltype(stack)> unwind(stack);
        stack.allocate(10, 1);
        unwind.unwind();
        REQUIRE(stack.top() == m);
        REQUIRE(unwind.will_unwind());

        {
            memory_stack_raii_unwind<decltype(stack)> unwind2(stack);
            stack.allocate(10, 1);
            unwind2.release();
            REQUIRE(!unwind2.will_unwind());
        }
        REQUIRE(stack.top() > m);
        m = stack.top();

        unwind.release(); // need to release
        unwind = memory_stack_raii_unwind<decltype(stack)>(stack);
        REQUIRE(unwind.will_unwind());
        REQUIRE(unwind.get_marker() == m);
        REQUIRE(&unwind.get_stack() == &stack);
        auto unwind2 = detail::move(unwind);
        REQUIRE(unwind2.will_unwind());
        REQUIRE(&unwind2.get_stack() == &stack);
        REQUIRE(unwind2.get_marker() == m);
        REQUIRE(!unwind.will_unwind());
    }
}
