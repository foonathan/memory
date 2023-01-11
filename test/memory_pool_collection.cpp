// Copyright (C) 2015-2023 Jonathan Müller and foonathan/memory contributors
// SPDX-License-Identifier: Zlib

#include "memory_pool_collection.hpp"

#include <algorithm>
#include <doctest/doctest.h>
#include <random>
#include <vector>

#include "allocator_storage.hpp"
#include "test_allocator.hpp"

using namespace foonathan::memory;
using namespace detail;

TEST_CASE("memory_pool_collection")
{
    using pools =
        memory_pool_collection<node_pool, identity_buckets, allocator_reference<test_allocator>>;
    test_allocator alloc;
    {
        const auto max_size = 16u;
        pools      pool(max_size, 4000, alloc);
        REQUIRE(pool.max_node_size() == max_size);
        REQUIRE(pool.capacity_left() <= 4000u);
        REQUIRE(pool.next_capacity() >= 4000u);
        REQUIRE(alloc.no_allocated() == 1u);

        for (auto i = 0u; i != max_size; ++i)
            REQUIRE(pool.pool_capacity_left(i) == 0u);

        SUBCASE("normal alloc/dealloc")
        {
            std::vector<void*> a, b;
            for (auto i = 0u; i != 5u; ++i)
            {
                a.push_back(pool.allocate_node(1));
                b.push_back(pool.try_allocate_node(5));
                REQUIRE(b.back());
            }
            REQUIRE(alloc.no_allocated() == 1u);
            REQUIRE(pool.capacity_left() <= 4000u);

            std::shuffle(a.begin(), a.end(), std::mt19937{});
            std::shuffle(b.begin(), b.end(), std::mt19937{});

            for (auto ptr : a)
                REQUIRE(pool.try_deallocate_node(ptr, 1));
            for (auto ptr : b)
                pool.deallocate_node(ptr, 5);
        }
        SUBCASE("single array alloc")
        {
            auto memory = pool.allocate_array(4, 4);
            pool.deallocate_array(memory, 4, 4);
        }
        SUBCASE("array alloc/dealloc")
        {
            std::vector<void*> a, b;
            for (auto i = 0u; i != 5u; ++i)
            {
                a.push_back(pool.allocate_array(4, 4));
                b.push_back(pool.try_allocate_array(5, 5));
                REQUIRE(b.back());
            }
            REQUIRE(alloc.no_allocated() == 1u);
            REQUIRE(pool.capacity_left() <= 4000u);

            std::shuffle(a.begin(), a.end(), std::mt19937{});
            std::shuffle(b.begin(), b.end(), std::mt19937{});

            for (auto ptr : a)
                REQUIRE(pool.try_deallocate_array(ptr, 4, 4));
            for (auto ptr : b)
                pool.deallocate_array(ptr, 5, 5);
        }
        SUBCASE("multiple block alloc/dealloc")
        {
            std::vector<void*> a, b;
            for (auto i = 0u; i != 1000u; ++i)
            {
                a.push_back(pool.allocate_node(1));
                b.push_back(pool.allocate_node(5));
            }
            REQUIRE(alloc.no_allocated() > 1u);

            std::shuffle(a.begin(), a.end(), std::mt19937{});
            std::shuffle(b.begin(), b.end(), std::mt19937{});

            for (auto ptr : a)
                pool.deallocate_node(ptr, 1);
            for (auto ptr : b)
                pool.deallocate_node(ptr, 5);
        }
    }
    REQUIRE(alloc.no_allocated() == 0u);
}
