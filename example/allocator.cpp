// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <iostream>
#include <unordered_set>
#include <vector>

#include "../allocator_adapter.hpp" // for raw_allocator_allocator
#include "../pool_allocator.hpp" // for memory_pool
#include "../tracking.hpp" // for make_tracked_allocator

using namespace foonathan;

// uses a RawAllocator or a class that has specialized the raw_allocator_traits
template <class RawAllocator>
void use_allocator(RawAllocator &alloc)
{
    // raw_allocator_allocator is the bridge between RawAllocators and STL-Allocator
    using std_allocator = memory::raw_allocator_allocator<int, RawAllocator>;
    
    // create a vector that uses the pool
    // it is passed via reference to the vector
    std::vector<int, std_allocator> a(alloc);
    // add some elements
    std::clog << "vector creation\n";
    for (auto i = 0u; i != 10u; ++i)
        a.push_back(i);
        
    // remove the third one
    std::clog << "vector erase\n";
    a.erase(a.begin() + 2);
    
    // make a copy of the vector
    // they share the same allocator
    std::clog << "vector copy\n";
    auto b = a;
    // insert an element into the copy
    std::clog << "vector insert\n";
    b.push_back(4);
    
    // move b into a new vector
    // the allocator is transferred two to make it fast
    std::clog << "vector move\n";
    auto c = std::move(b);
    
    // swap a and c, this also swaps the allocator
    std::clog << "vector swap\n";
    swap(a, c);
    
    // create a set that uses the allocator
    std::clog << "create set\n";
    std::unordered_set<int, std::hash<int>, std::equal_to<int>, std_allocator>
        set(5, {}, {}, alloc); // set bucket counter to trigger rehash
    
    // insert and erase values
    std::clog << "insert/erase set\n";
    for (auto i = 0u; i != 10u; ++i)
        set.insert(i);
    set.erase(2);
    set.erase(10);
}

int main()
{
    std::clog << std::unitbuf;
    // a memory pool that supports arrays, each node is 32 bytes big, initially 4KB long
    memory::memory_pool<memory::array_pool> pool(32, 4096);
    {
        // allocate one such node
        auto mem = pool.allocate_node();
        std::clog << "Allocated a node from pool " << mem << '\n';
        
        // deallocate it
        pool.deallocate_node(mem);
        
        // allocate an array, that is, multiple nodes one after the other
        mem = pool.allocate_array(16);
        std::clog << "Allocated array from pool " << mem << '\n';
        // deallocate it
        pool.deallocate_array(mem, 16);
    }
    // use the allocator inside STL containers
    std::clog << "\npool test\n\n";
    use_allocator(pool);
    
    // tracker class that logs internal behavior of the allocator
    struct tracker
    {
        void on_node_allocation(void *mem, std::size_t size, std::size_t) noexcept
        {
            std::clog << this << " node allocated: ";
            std::clog << mem << " (" << size << ") " << '\n';
        }
        
        void on_array_allocation(void *mem, std::size_t count, std::size_t size, std::size_t) noexcept
        {
            std::clog << this << " array allocated: ";
            std::clog << mem << " (" << count << " * " << size << ") " << '\n';
        }
        
        void on_node_deallocation(void *ptr, std::size_t, std::size_t) noexcept
        {
            std::clog << this << " node deallocated: " << ptr << " \n";
        }
        
        void on_array_deallocation(void *ptr, std::size_t, std::size_t, std::size_t) noexcept
        {
            std::clog << this << " array deallocated: " << ptr << " \n";
        }
    };
    
    // create a tracked memory_pool to see what kind of allocations are made
    // necessary to move the pool into the new allocator, since it can't be copied
    auto tracked_pool = memory::make_tracked_allocator(tracker{}, std::move(pool));
    // tracked_pool does not provide the pool specific interface anymore, only the general one
    
    // use it
    std::clog << "\ntracked pool test\n\n";
    use_allocator(tracked_pool);
}
