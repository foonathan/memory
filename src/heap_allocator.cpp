// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "heap_allocator.hpp"

#include <cstdlib>
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
            get_leak_handler()(FOONATHAN_MEMORY_IMPL_LOG_PREFIX "::heap_allocator", nullptr, alloc_counter);
    }
#else
    namespace
    {
        void on_alloc(std::size_t) FOONATHAN_NOEXCEPT {}
        void on_dealloc(std::size_t) FOONATHAN_NOEXCEPT {}
    }
#endif

void* heap_allocator::allocate_node(std::size_t size, std::size_t)
{
    auto memory = detail::try_allocate(std::malloc,
                                       size + 2 * detail::debug_fence_size,
                                       {FOONATHAN_MEMORY_IMPL_LOG_PREFIX "::heap_allocator", this});
    on_alloc(size);
    return detail::debug_fill_new(memory, size);
}

void heap_allocator::deallocate_node(void *ptr, std::size_t size, std::size_t) FOONATHAN_NOEXCEPT
{
    auto memory = detail::debug_fill_free(ptr, size);
    std::free(memory);

    on_dealloc(size);
}
