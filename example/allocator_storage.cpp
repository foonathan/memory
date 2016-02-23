// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

// this example shows how to store allocators by reference and type-erased
// see http://foonathan.github.io/doc/memory/md_doc_adapters_storage.html for further details

#include <iostream>
#include <memory>

#include <foonathan/memory/allocator_storage.hpp> // allocator_reference, any_allocator_reference
#include <foonathan/memory/heap_allocator.hpp> // heap_allocator
#include <foonathan/memory/memory_stack.hpp> // memory_stack

// alias namespace foonathan::memory as memory for easier access
#include <foonathan/memory/namespace_alias.hpp>

template <class RawAllocator>
void do_sth(memory::allocator_reference<RawAllocator> ref);

int main()
{
    // storing stateless allocator by reference
    // heap_allocator is stateless so it does not need to be actually referenced
    // the reference can take it as a temporary and construct it on the fly
    memory::allocator_reference<memory::heap_allocator> ref_stateless(memory::heap_allocator{});
    do_sth(ref_stateless);

    // create a memory_stack
    // allocates a memory block - initially 4KiB big - and allocates from it in a stack-like manner
    // deallocation is only done via unwinding to a previously queried marker
    memory::memory_stack<> stack(4096);

    // storing stateful allocator by reference
    // memory_stack is stateful and thus the reference actually takes the address of the object
    // the user has to ensure that the referenced object lives long enough
    memory::allocator_reference<memory::memory_stack<>> ref_stateful(stack);
    do_sth(ref_stateful);

    // storing a reference type-erased
    // any_allocator_reference is an alias for allocator_reference<any_allocator>
    // it triggers a specialization that uses type-erasure
    // the tag type can be passed to any class that uses an allocator_reference internally,
    // like std_allocator or the deep_copy_ptr from the other example
    // the empty template brackets are for the mutex that is used for synchronization (like in the normal reference),
    // the default is default_mutex and can be set via CMake options
    memory::any_allocator_reference<> any1(ref_stateful); // initialize with another allocator reference, will "unwrap"
    do_sth(any1);

    memory::any_allocator_reference<> any2(stack); // initialize with a "normal" RawAllocator
    do_sth(any2);

    memory::any_allocator_reference<> any3(std::allocator<char>{}); // normal Allocators are RawAllocators, too, so this works
    do_sth(any3);
}

template <class RawAllocator>
void do_sth(memory::allocator_reference<RawAllocator> ref)
{
    // ref is a full-blown RawAllocator that provides all member functions,
    // so there is no need to use the allocator_traits

    auto node = ref.allocate_node(sizeof(int), alignof(int));
    std::cout << "Got memory for an int " << node << '\n';
    ref.deallocate_node(node, sizeof(int), alignof(int));
}
