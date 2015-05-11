// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "heap_allocator.hpp"

#include <cstdlib>
#include <new>

#include "debugging.hpp"

using namespace foonathan::memory;

void* heap_allocator::allocate_node(std::size_t size, std::size_t)
{
    void* mem;
    while (true)
    {
        mem = std::malloc(size + 2 * detail::debug_fence_size);
        if (mem)
            break;
    #if FOONATHAN_IMPL_HAS_GET_NEW_HANDLER
        auto handler = std::get_new_handler();
    #else
        auto handler = std::set_new_handler(nullptr);
        std::set_new_handler(handler);
    #endif
        if (!handler)
            throw std::bad_alloc();
        handler();
    }
    auto memory = static_cast<char*>(mem);
    detail::debug_fill(memory, detail::debug_fence_size, debug_magic::fence_memory);
    memory += detail::debug_fence_size;
    detail::debug_fill(memory, size, debug_magic::new_memory);
    detail::debug_fill(memory + size, detail::debug_fence_size, debug_magic::fence_memory);
    return memory;
}

void heap_allocator::deallocate_node(void *ptr, std::size_t size, std::size_t) FOONATHAN_NOEXCEPT
{
    detail::debug_fill(ptr, size, debug_magic::freed_memory);
    auto memory = static_cast<char*>(ptr) - detail::debug_fence_size;
    std::free(memory);
}
