// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "fallback_allocator.hpp"

#include <catch.hpp>

#include "allocator_storage.hpp"
#include "test_allocator.hpp"

using namespace foonathan::memory;

TEST_CASE("fallback_allocator", "[adapter]")
{
    struct test_compositioning : test_allocator
    {
        bool fail = false;

        void* try_allocate_node(std::size_t size, std::size_t alignment)
        {
            return fail ? nullptr : allocate_node(size, alignment);
        }

        bool try_deallocate_node(void* ptr, std::size_t size, std::size_t alignment)
        {
            if (fail)
                return false;
            deallocate_node(ptr, size, alignment);
            return true;
        }
    } default_alloc;
    test_allocator fallback_alloc;

    using allocator = fallback_allocator<allocator_reference<test_compositioning>,
                                         allocator_reference<test_allocator>>;

    allocator alloc(default_alloc, fallback_alloc);
    REQUIRE(default_alloc.no_allocated() == 0u);
    REQUIRE(fallback_alloc.no_allocated() == 0u);

    auto ptr = alloc.allocate_node(1, 1);
    REQUIRE(default_alloc.no_allocated() == 1u);
    REQUIRE(fallback_alloc.no_allocated() == 0u);

    alloc.deallocate_node(ptr, 1, 1);
    REQUIRE(default_alloc.no_deallocated() == 1u);
    REQUIRE(fallback_alloc.no_deallocated() == 0u);

    default_alloc.fail = true;

    ptr = alloc.allocate_node(1, 1);
    REQUIRE(default_alloc.no_allocated() == 0u);
    REQUIRE(fallback_alloc.no_allocated() == 1u);

    alloc.deallocate_node(ptr, 1, 1);
    REQUIRE(default_alloc.no_deallocated() == 1u);
    REQUIRE(fallback_alloc.no_deallocated() == 1u);
}
