// Copyright (C) 2015-2021 Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "memory_resource_adapter.hpp"

#include <doctest/doctest.h>
#include <new>

#include "allocator_storage.hpp"
#include "std_allocator.hpp"

using namespace foonathan::memory;

struct pmr_test_allocator
{
    std::size_t node_allocated = 0, array_allocated = 0;

    void* allocate_node(std::size_t size, std::size_t)
    {
        node_allocated += size;
        return ::operator new(size);
    }

    void* allocate_array(std::size_t n, std::size_t size, std::size_t)
    {
        array_allocated += n * size;
        return ::operator new(n* size);
    }

    void deallocate_node(void* p, std::size_t size, std::size_t)
    {
        node_allocated -= size;
        ::operator delete(p);
    }

    void deallocate_array(void* p, std::size_t n, std::size_t size, std::size_t)
    {
        array_allocated -= n * size;
        ::operator delete(p);
    }

    std::size_t max_node_size() const
    {
        return 8u;
    }
};

TEST_CASE("memory_resource_adapter")
{
    auto max_node = pmr_test_allocator{}.max_node_size();

    memory_resource_adapter<pmr_test_allocator> alloc({});
    REQUIRE(alloc.get_allocator().node_allocated == 0u);
    REQUIRE(alloc.get_allocator().array_allocated == 0u);

    auto mem = alloc.allocate(max_node / 2);
    REQUIRE(alloc.get_allocator().node_allocated == max_node / 2);
    REQUIRE(alloc.get_allocator().array_allocated == 0u);

    alloc.deallocate(mem, max_node / 2);
    REQUIRE(alloc.get_allocator().node_allocated == 0);
    REQUIRE(alloc.get_allocator().array_allocated == 0u);

    mem = alloc.allocate(max_node);
    REQUIRE(alloc.get_allocator().node_allocated == max_node);
    REQUIRE(alloc.get_allocator().array_allocated == 0u);

    alloc.deallocate(mem, max_node);
    REQUIRE(alloc.get_allocator().node_allocated == 0);
    REQUIRE(alloc.get_allocator().array_allocated == 0u);

    mem = alloc.allocate(max_node * 2);
    REQUIRE(alloc.get_allocator().node_allocated == 0u);
    REQUIRE(alloc.get_allocator().array_allocated == max_node * 2);

    alloc.deallocate(mem, max_node * 2);
    REQUIRE(alloc.get_allocator().node_allocated == 0u);
    REQUIRE(alloc.get_allocator().array_allocated == 0u);

    mem = alloc.allocate(max_node * 2 + 1);
    REQUIRE(alloc.get_allocator().node_allocated == 0u);
    REQUIRE(alloc.get_allocator().array_allocated == max_node * 3);

    alloc.deallocate(mem, max_node * 2 + 1);
    REQUIRE(alloc.get_allocator().node_allocated == 0u);
    REQUIRE(alloc.get_allocator().array_allocated == 0u);
}

// compilation checks
template class foonathan::memory::allocator_storage<reference_storage<memory_resource_allocator>,
                                                    no_mutex>;
template class foonathan::memory::allocator_traits<memory_resource_allocator>;
