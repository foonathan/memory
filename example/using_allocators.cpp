// Copyright (C) 2015 Jonathan M�ller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

// this examples shows the basic usage of RawAllocator classes with containers and smart pointers
// see http://foonathan.github.io/doc/memory/md_doc_external_usage.html for more details

#include <algorithm>
#include <iostream>
#include <iterator>

#include <memory/container.hpp> // vector, list, list_node_size
#include <memory/memory_pool.hpp> // memory_pool
#include <memory/smart_ptr.hpp> // allocate_unique
#include <memory/temporary_allocator.hpp> // temporary_allocator

// namespace alias for easier access
// if the CMake option FOONATHAN_MEMORY_NAMESPACE_PREFIX is OFF (the default),
// it is already provided in each header file, so unnecessary in real application code
namespace memory = foonathan::memory;

template <typename BiIter>
void merge_sort(BiIter begin, BiIter end);

int main()
{
    // a memory pool RawAllocator
    // allocates a memory block - initially 4KiB - and splits it into chunks of list_node_size<int>::value big
    // list_node_size<int>::value is the size of each node of a std::list
    memory::memory_pool<> pool(memory::list_node_size<int>::value, 4096u);

    // alias for std::list<int, memory::std_allocator<int, memory::memory_pool<>>
    // a std::list using a memory_pool
    // std_allocator stores a reference to a RawAllocator and provides the Allocator interface
    memory::list<int, std::allocator<char>> list(std::allocator<char>{});
    list.push_back(3);
    list.push_back(2);
    list.push_back(1);

    merge_sort(list.begin(), list.end());

    // allocate a std::unique_ptr using the pool
    // memory::allocate_shared is also available
    auto ptr = memory::allocate_unique<int>(pool, *list.begin());
    std::cout << *ptr << '\n';
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