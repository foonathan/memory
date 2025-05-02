// Copyright (C) 2015-2025 Jonathan Müller and foonathan/memory contributors
// SPDX-License-Identifier: Zlib

// this example shows how to track allocations
// see https://memory.foonathan.net/md_doc_adapters_storage.html for further details

#include <iostream>

#include <foonathan/memory/container.hpp>   // set, set_node_size
#include <foonathan/memory/memory_pool.hpp> // memory_pool
#include <foonathan/memory/tracking.hpp>    // make_tracked_allocator

// alias namespace foonathan::memory as memory for easier access
#include <foonathan/memory/namespace_alias.hpp>

int main()
{
    using namespace memory::literals;

    // tracker class that logs internal behavior of the allocator
    struct tracker
    {
        void on_node_allocation(void* mem, std::size_t size, std::size_t) noexcept
        {
            std::clog << this << " node allocated: ";
            std::clog << mem << " (" << size << ") " << '\n';
        }

        void on_array_allocation(void* mem, std::size_t count, std::size_t size,
                                 std::size_t) noexcept
        {
            std::clog << this << " array allocated: ";
            std::clog << mem << " (" << count << " * " << size << ") " << '\n';
        }

        void on_node_deallocation(void* ptr, std::size_t, std::size_t) noexcept
        {
            std::clog << this << " node deallocated: " << ptr << " \n";
        }

        void on_array_deallocation(void* ptr, std::size_t, std::size_t, std::size_t) noexcept
        {
            std::clog << this << " array deallocated: " << ptr << " \n";
        }
    };

    {
        // create a tracked default allocator
        auto tracked_allocator =
            memory::make_tracked_allocator(tracker{}, memory::default_allocator{});

        // use the allocator as usual
        // decltype(tracked_allocator) can be used below, too
        memory::vector<int, memory::tracked_allocator<tracker, memory::default_allocator>>
            vec({1, 2, 3, 4}, tracked_allocator);

        std::clog << "vec: ";
        for (auto i : vec)
            std::clog << i << ' ';
        std::clog << '\n';
    }

    {
        // create a tracked memory_pool to see what kind of allocations are made
        auto tracked_pool =
            memory::make_tracked_allocator(tracker{},
                                           memory::memory_pool<>(memory::set_node_size<int>::value,
                                                                 4_KiB));

        // use the allocator as usual
        // decltype(tracked_pool) can be used below, too
        memory::set<int, memory::tracked_allocator<tracker, memory::memory_pool<>>>
            set(std::less<int>(), tracked_pool);

        set.insert(1);
        set.insert(2);
        set.insert(3);
        set.insert(1);

        std::clog << "set: ";
        for (auto i : set)
            std::clog << i << ' ';
        std::clog << '\n';

        set.erase(2);
    }
}
