// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <algorithm>
#include <iostream>
#include <vector>

#include "../smart_ptr.hpp" // raw_allocate_unique
#include "../temporary_allocator.hpp" // temporary_allocator & co

using namespace foonathan;

template <typename RAIter>
void merge_sort(RAIter begin, RAIter end);

int main()
{
    // creates a temporary allocator, it allows fast allocation for temporary memory
    // it is similar to alloca but uses a memory_stack instead of the real stack
    // each thread has its own internal memory_stack
    // it is created on the first call to this function where you can optionally specify the stack size
    // it is destroyed when the thread ends
    auto alloc = memory::make_temporary_allocator();
    
    // directly allocate memory
    // it will be freed when alloc goes out of scope
    std::cout << "Allocated: " << alloc.allocate(sizeof(int), FOONATHAN_ALIGNOF(int)) << '\n';
    
    // create temporary array of 5 elements
    auto array = memory::raw_allocate_unique<int[]>(alloc, 5);
    array[0] = 4;
    array[1] = 2;
    array[2] = 5;
    array[3] = 1;
    array[4] = 3;
    
    merge_sort(array.get(), array.get() + 5);
    
    std::cout << "Sorted: ";
    for (auto i = 0u; i != 5; ++i)
        std::cout << array[i] << ' ';
    std::cout << '\n';
}

// naive implementation of merge_sort using temporary memory allocator
template <typename RAIter>
void merge_sort(RAIter begin, RAIter end)
{
    using value_type = typename std::iterator_traits<RAIter>::value_type;
    using vector_t = std::vector<value_type,
                    memory::raw_allocator_allocator<value_type, memory::temporary_allocator>>;
    
    if (end - begin <= 1)
        return;
    auto mid = begin + (end - begin) / 2;
    
    // create the vectors using temporary_allocator for fast memory allocation
    // since the internal stack is already created in main() this does not invovle heap allocation
    auto alloc = memory::make_temporary_allocator();
    vector_t first(begin, mid, alloc), second(mid, end, alloc);
    
    merge_sort(first.begin(), first.end());
    merge_sort(second.begin(), second.end());
    std::merge(first.begin(), first.end(), second.begin(), second.end(), begin);
}
