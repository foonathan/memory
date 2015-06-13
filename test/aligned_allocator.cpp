// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "aligned_allocator.hpp"

#include <catch.hpp>

#include "detail/align.hpp"
#include "allocator_adapter.hpp"
#include "stack_allocator.hpp"

using namespace foonathan::memory;

TEST_CASE("aligned_allocator", "[adapter]")
{
    using allocator_t = aligned_allocator<allocator_reference<memory_stack<>>>;

    memory_stack<> stack(1024);
    stack.allocate(3, 1); // manual misalign

    allocator_t alloc(4u, stack);
    REQUIRE(alloc.min_alignment() == 4u);

    auto mem1 = alloc.allocate_node(16u, 1u);
    REQUIRE(detail::align_offset(mem1, 4u) == 0u);
    auto mem2 = alloc.allocate_node(16u, 8u);
    REQUIRE(detail::align_offset(mem2, 4u) == 0u);
    REQUIRE(detail::align_offset(mem2, 8u) == 0u);

    alloc.deallocate_node(mem2, 16u, 8u);
    alloc.deallocate_node(mem1, 16u, 1u);
}
