// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "memory_pool_collection.hpp"

#include <algorithm>
#include <catch.hpp>
#include <random>
#include <vector>

#include "allocator_storage.hpp"
#include "test_allocator.hpp"

using namespace foonathan::memory;
using namespace detail;

TEST_CASE("memory_pool_collection", "[pool]")
{
    using pools = memory_pool_collection<node_pool, identity_buckets,
                                        allocator_reference<test_allocator>>;
    test_allocator alloc;
    {
        const auto max_size = 16u;
        pools pool(max_size, 1000, alloc);
        REQUIRE(pool.max_node_size() == max_size);
        REQUIRE(pool.capacity_left() <= 1000u);
        REQUIRE(pool.next_capacity() >= 1000u);
        REQUIRE(alloc.no_allocated() == 1u);

        for (auto i = 0u; i != max_size; ++i)
            REQUIRE(pool.pool_capacity_left(i) == 0u);

        SECTION("normal alloc/dealloc")
        {
            std::vector<void*> a, b;
            for (auto i = 0u; i != 5u; ++i)
            {
                a.push_back(pool.allocate_node(1));
                b.push_back(pool.allocate_node(5));
            }
            REQUIRE(alloc.no_allocated() == 1u);
            REQUIRE(pool.capacity_left() <= 1000u);

            std::shuffle(a.begin(), a.end(), std::mt19937{});
            std::shuffle(b.begin(), b.end(), std::mt19937{});

            for (auto ptr : a)
                pool.deallocate_node(ptr, 1);
            for (auto ptr : b)
                pool.deallocate_node(ptr, 5);
        }
        SECTION("multiple block alloc/dealloc")
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
