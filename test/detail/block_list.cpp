// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "detail/block_list.hpp"

#include <catch.hpp>

#include "allocator_storage.hpp"
#include "../test_allocator.hpp"

using namespace foonathan::memory;
using namespace detail;

TEST_CASE("detail::block_list_impl", "[detail][core]")
{
    block_list_impl list;
    REQUIRE(list.empty());

    SECTION("push/pop/top")
    {
        alignas(max_alignment) char memory[1024];
        void *mem = memory;
        auto offset = list.push(mem, 1024);
        REQUIRE(offset == block_list_impl::impl_offset());
        auto diff = static_cast<char*>(mem) - memory;
        REQUIRE(diff == offset);
        REQUIRE(!list.empty());

        auto block = list.top();
        REQUIRE(block.size == 1024 - offset);
        REQUIRE(block.memory == mem);

        block = list.pop();
        REQUIRE(block.size == 1024);
        REQUIRE(block.memory == static_cast<void*>(memory));
    }
    SECTION("multiple lists")
    {
        alignas(max_alignment) char memory[1024];
        void *mem = memory;
        list.push(mem, 1024);

        block_list_impl another_list;
        auto block = another_list.push(list);
        REQUIRE(block.size == 1024 - block_list_impl::impl_offset());
        REQUIRE(block.memory == mem);
        REQUIRE(another_list.top().size == block.size);
        REQUIRE(another_list.top().memory == block.memory);
        REQUIRE(list.empty());
    }

    alignas(max_alignment) char a[1024], b[1024], c[1024];
    void* mem = a;
    list.push(mem, 1024);
    mem = b;
    list.push(mem, 1024);
    mem = c;
    list.push(mem, 1024);

    SECTION("multiple pop")
    {
        auto block = list.pop();
        REQUIRE(block.memory == static_cast<void*>(c));
        block = list.pop();
        REQUIRE(block.memory == static_cast<void*>(b));
        block = list.pop();
        REQUIRE(block.memory == static_cast<void*>(a));
        REQUIRE(list.empty());
    }
    SECTION("move")
    {
        alignas(max_alignment) char memory[1024];
        mem = memory;
        list.push(mem, 1024);

        auto another_list = detail::move(list);
        REQUIRE(!another_list.empty());
        REQUIRE(list.empty());
        REQUIRE(another_list.top().memory == mem);

        alignas(max_alignment) char memory2[1024];
        mem = memory2;
        another_list.push(mem, 1024);
        REQUIRE(another_list.top().memory == mem);

        alignas(max_alignment) char memory3[1024];
        mem = memory3;
        list.push(mem, 1024);
        REQUIRE(!list.empty());
        REQUIRE(list.top().memory == mem);

        list = detail::move(another_list);
        REQUIRE(another_list.empty());
        REQUIRE(!list.empty());
        REQUIRE(list.pop().memory == memory2);
        REQUIRE(list.pop().memory == memory);
        REQUIRE(list.pop().memory == c);
    }
}

TEST_CASE("detail::block_list", "[detail][core]")
{
    using list_t = block_list<allocator_reference<test_allocator>>;

    test_allocator alloc;
    {
        list_t list(1024u, alloc);
        REQUIRE(list.size() == 0u);

        auto size = list.next_block_size() + block_list_impl::impl_offset();
        REQUIRE(size == 1024u);
        auto block = list.allocate();
        REQUIRE(list.size() == 1u);
        void* memory = static_cast<char*>(block.memory) - block_list_impl::impl_offset();
        REQUIRE(alloc.last_allocated().memory == memory);
        REQUIRE(alloc.last_allocated().size == size);
        REQUIRE(alloc.last_allocated().alignment == max_alignment);

        auto top = list.top();
        REQUIRE(top.memory == block.memory);
        REQUIRE(top.size == block.size);

        list.deallocate();
        REQUIRE(list.size() == 0u);
        REQUIRE(alloc.no_deallocated() == 0u);

        list.shrink_to_fit();
        REQUIRE(alloc.no_deallocated() == 1u);
        REQUIRE(alloc.last_deallocation_valid());
        REQUIRE(alloc.no_allocated() == 0u);

        list.allocate();
        list.deallocate();
        REQUIRE(alloc.no_allocated() == 1);
        list.allocate();
        REQUIRE(alloc.no_allocated() == 1);

    }
    REQUIRE(alloc.no_allocated() == 0u);
    {
        list_t a(1024, alloc);
        for (std::size_t i = 0u; i != 10u; ++i)
            a.allocate();
        REQUIRE(a.size() == 10u);
        REQUIRE(alloc.no_allocated() == 10u);
        auto b = detail::move(a);
        REQUIRE(a.size() == 0u);
        REQUIRE(b.size() == 10u);
        for (std::size_t i = 0u; i != 10u; ++i)
            b.deallocate();
        REQUIRE(alloc.no_allocated() == 10u);
        REQUIRE(b.size() == 0u);

        a.allocate();
        a.allocate();
        REQUIRE(a.size() == 2u);
        b = detail::move(a);
        REQUIRE(b.size() == 2u);
        REQUIRE(a.size() == 0u);
    }
    REQUIRE(alloc.last_deallocation_valid());
    REQUIRE(alloc.no_allocated() == 0u);
}
