// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "iteration_allocator.hpp"

#include <catch.hpp>

#include "allocator_storage.hpp"
#include "test_allocator.hpp"

using namespace foonathan::memory;

TEST_CASE("iteration_allocator", "[stack]")
{
    SECTION("basic")
    {
        test_allocator                                              alloc;
        iteration_allocator<2, allocator_reference<test_allocator>> iter_alloc(100, alloc);
        REQUIRE(alloc.no_allocated() == 1u);
        REQUIRE(iter_alloc.max_iterations() == 2u);
        REQUIRE(iter_alloc.cur_iteration() == 0u);
        REQUIRE(iter_alloc.capacity_left(0u) == 50);
        REQUIRE(iter_alloc.capacity_left(1u) == 50);

        iter_alloc.allocate(10, 1);
        REQUIRE(iter_alloc.capacity_left() < 50);
        iter_alloc.allocate(4, 4);
        REQUIRE(iter_alloc.capacity_left() < 50);

        REQUIRE(iter_alloc.capacity_left(1u) == 50);
        iter_alloc.next_iteration();
        REQUIRE(iter_alloc.cur_iteration() == 1u);
        REQUIRE(iter_alloc.capacity_left() == 50);
        REQUIRE(iter_alloc.capacity_left(0u) < 50);

        iter_alloc.allocate(10, 1);
        REQUIRE(iter_alloc.capacity_left() < 50);

        iter_alloc.next_iteration();
        REQUIRE(iter_alloc.cur_iteration() == 0u);
        REQUIRE(iter_alloc.capacity_left() == 50);
        REQUIRE(iter_alloc.capacity_left(1u) < 50);

        iter_alloc.next_iteration();
        REQUIRE(iter_alloc.cur_iteration() == 1u);
        REQUIRE(iter_alloc.capacity_left() == 50);
    }
    SECTION("overaligned")
    {
        test_allocator                                              alloc;
        iteration_allocator<1, allocator_reference<test_allocator>> iter_alloc(100, alloc);

        auto align = 2 * detail::max_alignment;
        auto mem   = iter_alloc.allocate(align, align);
        REQUIRE(detail::is_aligned(mem, align));
    }
}
