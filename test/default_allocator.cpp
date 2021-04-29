// Copyright (C) 2015-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

// tests all possible default allocator classes

#include "default_allocator.hpp"

#include <doctest/doctest.h>
#include <vector>

#include "detail/align.hpp"

using namespace foonathan::memory;

// *very* simple test case to ensure proper alignment and might catch some segfaults
template <class Allocator>
void check_default_allocator(Allocator& alloc, std::size_t def_alignment = detail::max_alignment)
{
    auto ptr = alloc.allocate_node(1, 1);
    REQUIRE(detail::is_aligned(ptr, def_alignment));

    alloc.deallocate_node(ptr, 1, 1);

    for (std::size_t i = 0u; i != 10u; ++i)
    {
        auto node = alloc.allocate_node(i, 1);
        REQUIRE(detail::is_aligned(node, def_alignment));
        alloc.deallocate_node(node, i, 1);
    }

    std::vector<void*> nodes;
    for (std::size_t i = 0u; i != 10u; ++i)
    {
        auto node = alloc.allocate_node(i, 1);
        REQUIRE(detail::is_aligned(node, def_alignment));
        nodes.push_back(node);
    }

    for (std::size_t i = 0u; i != 10u; ++i)
        alloc.deallocate_node(nodes[i], i, 1);
}

TEST_CASE("heap_allocator")
{
    heap_allocator alloc;
    check_default_allocator(alloc);
}

TEST_CASE("new_allocator")
{
    new_allocator alloc;
    check_default_allocator(alloc);
}

TEST_CASE("malloc_allocator")
{
    malloc_allocator alloc;
    check_default_allocator(alloc);
}

TEST_CASE("static_allocator")
{
    static_allocator_storage<1024> storage;
    static_allocator               alloc(storage);

    // no need to test alignment issues here again, implemented by fixed_memory_stack
    check_default_allocator(alloc, 1);
}

TEST_CASE("virtual_memory_allocator")
{
    virtual_memory_allocator alloc;
    check_default_allocator(alloc, virtual_memory_page_size);
}
