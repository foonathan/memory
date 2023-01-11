// Copyright (C) 2015-2023 Jonathan Müller and foonathan/memory contributors
// SPDX-License-Identifier: Zlib

#include "smart_ptr.hpp"

#include <doctest/doctest.h>

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

TEST_CASE("allocate_shared")
{
    SUBCASE("stateless")
    {
        dummy_allocator::size = 0;
        auto ptr              = allocate_shared<int>(dummy_allocator{}, 42);
        REQUIRE(*ptr == 42);
#if !defined(FOONATHAN_MEMORY_NO_NODE_SIZE)
        REQUIRE((dummy_allocator::size <= allocate_shared_node_size<int, dummy_allocator>::value));
#endif
    }
    SUBCASE("stateful")
    {
#if defined(FOONATHAN_MEMORY_NO_NODE_SIZE)
        memory_pool<> pool(128, 1024); // hope that's enough
#else
        memory_pool<> pool(allocate_shared_node_size<int, memory_pool<>>::value, 1024);
#endif
        auto ptr = allocate_shared<int>(pool, 42);
        REQUIRE(*ptr == 42);
    }
}
