// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

// this examples shows the basic usage of RawAllocator classes with containers and smart pointers
// see http://foonathan.github.io/doc/memory/md_doc_external_usage.html for more details

#include <algorithm>
#include <iostream>
#include <iterator>

#include <foonathan/memory/container.hpp> // vector, list, list_node_size,...
#include <foonathan/memory/memory_pool.hpp> // memory_pool
#include <foonathan/memory/smart_ptr.hpp> // allocate_unique
#include <foonathan/memory/static_allocator.hpp> // static_allocator_storage, static_block_allocator
#include <foonathan/memory/temporary_allocator.hpp> // temporary_allocator

// alias namespace foonathan::memory as memory for easier access
#include <foonathan/memory/namespace_alias.hpp>

template <typename BiIter>
void merge_sort(BiIter begin, BiIter end);

int main()
{
    // a memory pool RawAllocator
    // allocates a memory block - initially 4KiB - and splits it into chunks of list_node_size<int>::value big
    // list_node_size<int>::value is the size of each node of a std::list
    memory::memory_pool<> pool(memory::list_node_size<int>::value, 4096u);

    // just an alias for std::list<int, memory::std_allocator<int, memory::memory_pool<>>
    // a std::list using a memory_pool
    // std_allocator stores a reference to a RawAllocator and provides the Allocator interface
    memory::list<int, memory::memory_pool<>> list(pool);
    list.push_back(3);
    list.push_back(2);
    list.push_back(1);

    for (auto e : list)
        std::cout << e << ' ';
    std::cout << '\n';

    merge_sort(list.begin(), list.end());

    for (auto e : list)
        std::cout << e << ' ';
    std::cout << '\n';

    // allocate a std::unique_ptr using the pool
    // memory::allocate_shared is also available
    auto ptr = memory::allocate_unique<int>(pool, *list.begin());
    std::cout << *ptr << '\n';

    // static storage of size 4KiB
    memory::static_allocator_storage<4096u> storage;

    // a memory pool again but this time with a BlockAllocator
    // this controls the internal allocations of the pool itself
    // we need to specify the first template parameter giving the type of the pool as well
    // (node_pool is the default)
    // we use a static_block_allocator that uses the static storage above
    // all allocations will use a memory block on the stack
    using static_pool_t = memory::memory_pool<memory::node_pool, memory::static_block_allocator>;
    static_pool_t static_pool(memory::unordered_set_node_size<int>::value, 4096u, storage);

    // again, just an alias for std::unordered_set<int, std::hash<int>, std::equal_to<int>, memory::std_allocator<int, static_pool_t>
    // see why I wrote these? :D
    // now we have a hash set that lives on the stack!
    memory::unordered_set<int, static_pool_t> set(13, std::hash<int>{}, std::equal_to<int>{}, static_pool); // GCC 4.7 is missing the allocator-only ctor, breaks travis :(

    set.insert(3);
    set.insert(2);
    set.insert(3); // running out of stack memory is properly handled, of course

    for (auto e : set)
        std::cout << e << ' ';
    std::cout << '\n';
}

// naive implementation of merge_sort using temporary memory allocator
template <typename BiIter>
void merge_sort(BiIter begin, BiIter end)
{
    using value_type = typename std::iterator_traits<BiIter>::value_type;

    auto distance = std::distance(begin, end);
    if (distance <= 1)
        return;

    auto mid = begin;
    std::advance(mid, distance / 2);

    // an allocator for temporary memory
    // is similar to alloca() but uses its own stack
    // this stack is thread_local and created on the first call to this function
    // as soon as the allocator object goes out of scope, everything allocated through it, will be freed
    auto alloc = memory::make_temporary_allocator();

    // alias for std::vector<value_type, memory::std_allocator<value_type, memory::temporary_allocator>>
    // a std::vector using a temporary_allocator
    memory::vector<value_type, memory::temporary_allocator> first(begin, mid, alloc),
                                                            second(mid, end, alloc);

    merge_sort(first.begin(), first.end());
    merge_sort(second.begin(), second.end());
    std::merge(first.begin(), first.end(), second.begin(), second.end(), begin);
}