// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <iostream>

#include "../smart_ptr.hpp" // raw_allocate_unique/shared
#include "../stack_allocator.hpp" // memory_stack

using namespace foonathan;

void func(const std::shared_ptr<int> &ptr)
{
    std::cout << *ptr << '\n';
    *ptr = 10;
}

int main()
{
    // create a memory stack initially 4KB big
    memory::memory_stack<> stack(4096);
    
    // create a shared pointer
    // special deleter takes care of deallocation
    auto sptr = memory::raw_allocate_shared<int>(stack, 5);
    func(sptr);
    
    // create marker for stack unwinding
    auto m = stack.top();
    for (auto i = 0u; i != 10; ++i)
    {
        // free all memory from previous iteration
        stack.unwind(m);
        
        // create a unique pointer to a single int
        // when ptr goes out of scope, its special deleter will call the appropriate deallocate function
        // this is a no-op for memory_stack but for other allocator types it works
        auto ptr = memory::raw_allocate_unique<int>(stack, i);
        std::cout << *ptr << '\n';
        
        // create a unique pointer for an array of size 3
        auto array = memory::raw_allocate_unique<int[]>(stack, 3);
        array[2] = 5;
    }
}
