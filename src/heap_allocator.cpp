// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "heap_allocator.hpp"

#include <new>

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

    detail::heap_allocator_leak_checker_initializer_t::
        heap_allocator_leak_checker_initializer_t() FOONATHAN_NOEXCEPT
    {
        ++init_counter;
    }

    detail::heap_allocator_leak_checker_initializer_t::
        ~heap_allocator_leak_checker_initializer_t() FOONATHAN_NOEXCEPT
    {
        if (--init_counter == 0u && alloc_counter != 0u)
            get_leak_handler()({FOONATHAN_MEMORY_LOG_PREFIX "::heap_allocator", nullptr}, alloc_counter);
    }
#else
    namespace
    {
        void on_alloc(std::size_t) FOONATHAN_NOEXCEPT {}
        void on_dealloc(std::size_t) FOONATHAN_NOEXCEPT {}
    }
#endif

#if FOONATHAN_HOSTED_IMPLEMENTATION
    #include <cstdlib>
    #include <memory>

    void* foonathan::memory::malloc(std::size_t size) FOONATHAN_NOEXCEPT
    {
        return std::malloc(size);
    }

    void foonathan::memory::free(void *ptr, std::size_t) FOONATHAN_NOEXCEPT
    {
        std::free(ptr);
    }

    namespace
    {
        std::size_t max_size() FOONATHAN_NOEXCEPT
        {
            return std::allocator<char>().max_size();
        }
    }
#else
    // no implemenetation for malloc/free

    namespace
    {
        std::size_t max_size() FOONATHAN_NOEXCEPT
        {
            return std::size_t(-1);
        }
    }
#endif

void* heap_allocator::allocate_node(std::size_t size, std::size_t)
{
    auto memory = detail::try_allocate(memory::malloc,
                                       size + 2 * detail::debug_fence_size,
                                       {FOONATHAN_MEMORY_LOG_PREFIX "::heap_allocator", this});
    on_alloc(size);
    return detail::debug_fill_new(memory, size);
}

void heap_allocator::deallocate_node(void *ptr, std::size_t size, std::size_t) FOONATHAN_NOEXCEPT
{
    auto memory = detail::debug_fill_free(ptr, size);
    memory::free(memory, size);

    on_dealloc(size);
}

std::size_t heap_allocator::max_node_size() const FOONATHAN_NOEXCEPT
{
    return max_size();
}
