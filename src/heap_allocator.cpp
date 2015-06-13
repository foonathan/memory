// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "heap_allocator.hpp"

#include <cstdlib>
#include <new>

#include "debugging.hpp"

using namespace foonathan::memory;

#if FOONATHAN_MEMORY_DEBUG_LEAK_CHECK
#include <atomic>

namespace
{
    std::size_t init_counter = 0u, alloc_counter = 0u;
}

detail::heap_allocator_leak_checker_initializer_t::heap_allocator_leak_checker_initializer_t() FOONATHAN_NOEXCEPT
{
    ++init_counter;
}

detail::heap_allocator_leak_checker_initializer_t::~heap_allocator_leak_checker_initializer_t() FOONATHAN_NOEXCEPT
{
    if (--init_counter == 0u && alloc_counter != 0u)
        get_leak_handler()("foonathan::memory::heap_allocator", nullptr, alloc_counter);
}
#endif

void* heap_allocator::allocate_node(std::size_t size, std::size_t)
{
    void* mem;
    while (true)
    {
        mem = std::malloc(size + 2 * detail::debug_fence_size);
        if (mem)
            break;
    #if FOONATHAN_HAS_GET_NEW_HANDLER
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
#if FOONATHAN_MEMORY_DEBUG_LEAK_CHECK
    alloc_counter += size;
#endif
    return memory;
}

void heap_allocator::deallocate_node(void *ptr, std::size_t size, std::size_t) FOONATHAN_NOEXCEPT
{
    detail::debug_fill(ptr, size, debug_magic::freed_memory);
    auto memory = static_cast<char*>(ptr) - detail::debug_fence_size;
    std::free(memory);
#if FOONATHAN_MEMORY_DEBUG_LEAK_CHECK
    alloc_counter -= size;
#endif
}
