// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "config.hpp"
#if FOONATHAN_HOSTED_IMPLEMENTATION

#include "malloc_allocator.hpp"

#include <cstdlib>
#include <memory>

#include "debugging.hpp"
#include "error.hpp"

using namespace foonathan::memory;

#if FOONATHAN_MEMORY_DEBUG_LEAK_CHECK
    #include <atomic>

    namespace
    {
        std::size_t init_counter = 0u, alloc_counter = 0u;

        void on_alloc(std::size_t size) FOONATHAN_NOEXCEPT
        {
            alloc_counter += size;
        }

        void on_dealloc(std::size_t size) FOONATHAN_NOEXCEPT
        {
            alloc_counter -= size;
        }
    }

    detail::malloc_allocator_leak_checker_initializer_t::malloc_allocator_leak_checker_initializer_t() FOONATHAN_NOEXCEPT
    {
        ++init_counter;
    }

    detail::malloc_allocator_leak_checker_initializer_t::~malloc_allocator_leak_checker_initializer_t() FOONATHAN_NOEXCEPT
    {
        if (--init_counter == 0u && alloc_counter != 0u)
            get_leak_handler()({FOONATHAN_MEMORY_LOG_PREFIX "::malloc_allocator", nullptr}, alloc_counter);
    }
#else
    namespace
    {
        void on_alloc(std::size_t) FOONATHAN_NOEXCEPT {}
        void on_dealloc(std::size_t) FOONATHAN_NOEXCEPT {}
    }
#endif

void* malloc_allocator::allocate_node(std::size_t size, std::size_t)
{
    auto memory = std::malloc(size + 2 * detail::debug_fence_size);
    if (!memory)
        FOONATHAN_THROW(out_of_memory({FOONATHAN_MEMORY_LOG_PREFIX "::malloc_allocator", this},
                                      size + 2 * detail::debug_fence_size));
    on_alloc(size);
    return detail::debug_fill_new(memory, size);
}

void malloc_allocator::deallocate_node(void *ptr, std::size_t size, std::size_t) FOONATHAN_NOEXCEPT
{
    auto memory = detail::debug_fill_free(ptr, size);
    std::free(memory);

    on_dealloc(size);
}

std::size_t malloc_allocator::max_node_size() const FOONATHAN_NOEXCEPT
{
    return std::allocator<char>().max_size();
}

#endif