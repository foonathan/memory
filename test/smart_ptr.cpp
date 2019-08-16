// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "smart_ptr.hpp"

#include <catch.hpp>

#include "container.hpp"
#include "memory_pool.hpp"

using namespace foonathan::memory;

struct dummy_allocator
{
    static std::size_t size;

    void* allocate_node(std::size_t s, std::size_t)
    {
        size = s;
        return ::operator new(size);
    }

    void deallocate_node(void* ptr, std::size_t, std::size_t)
    {
        ::operator delete(ptr);
    }
};

std::size_t dummy_allocator::size = 0;

TEST_CASE("allocate_shared", "[adapter]")
{
    SECTION("stateless")
    {
        dummy_allocator::size = 0;
        auto ptr              = allocate_shared<int>(dummy_allocator{}, 42);
        REQUIRE(*ptr == 42);
        REQUIRE((dummy_allocator::size <= allocate_shared_node_size<int, dummy_allocator>::value));
    }
    SECTION("stateful no mutex")
    {
        memory_pool<> pool(allocate_shared_node_size<int, memory_pool<>, no_mutex>::value, 1024);
        auto          ptr = allocate_shared<int, no_mutex>(pool, 42);
        REQUIRE(*ptr == 42);
    }
    SECTION("stateful mutex")
    {
        memory_pool<> pool(allocate_shared_node_size<int, memory_pool<>>::value, 1024);
        auto          ptr = allocate_shared<int>(pool, 42);
        REQUIRE(*ptr == 42);
    }
}
