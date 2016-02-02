// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "new_allocator.hpp"

#if FOONATHAN_HOSTED_IMPLEMENTATION
    #include <memory>
#endif

#include "detail/align.hpp"
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

    detail::new_allocator_leak_checker_initializer_t::new_allocator_leak_checker_initializer_t() FOONATHAN_NOEXCEPT
    {
        ++init_counter;
    }

    detail::new_allocator_leak_checker_initializer_t::~new_allocator_leak_checker_initializer_t() FOONATHAN_NOEXCEPT
    {
        if (--init_counter == 0u && alloc_counter != 0u)
            get_leak_handler()({FOONATHAN_MEMORY_LOG_PREFIX "::new_allocator", nullptr}, alloc_counter);
    }
#else
    namespace
    {
        void on_alloc(std::size_t) FOONATHAN_NOEXCEPT {}
        void on_dealloc(std::size_t) FOONATHAN_NOEXCEPT {}
    }
#endif

void* new_allocator::allocate_node(std::size_t size, std::size_t)
{
    auto actual_size = size + (detail::debug_fence_size ? 2 * detail::max_alignment : 0u);

    auto mem = detail::try_allocate([](std::size_t size)
                                    {
                                        return ::operator new(size,
                                                              std::nothrow);
                                    }, actual_size,
                                    {FOONATHAN_MEMORY_LOG_PREFIX "::new_allocator", this});
    on_alloc(size);
    return detail::debug_fill_new(mem, size, detail::max_alignment);
}

void new_allocator::deallocate_node(void* node, std::size_t size, std::size_t) FOONATHAN_NOEXCEPT
{
    auto memory = detail::debug_fill_free(node, size, detail::max_alignment);
    ::operator delete(memory);

    on_dealloc(size);
}

std::size_t new_allocator::max_node_size() const FOONATHAN_NOEXCEPT
{
#if FOONATHAN_HOSTED_IMPLEMENTATION
    return std::allocator<char>().max_size();
#else
    return -1;
#endif
}

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
    template class allocator_traits<new_allocator>;
#endif
