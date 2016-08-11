// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "debugging.hpp"

#if FOONATHAN_HOSTED_IMPLEMENTATION
#include <cstdio>
#endif

#include <atomic>

#include "error.hpp"

using namespace foonathan::memory;

namespace
{
    void default_leak_handler(const allocator_info& info, std::ptrdiff_t amount) FOONATHAN_NOEXCEPT
    {
#if FOONATHAN_HOSTED_IMPLEMENTATION
        if (amount > 0)
            std::fprintf(stderr, "[%s] Allocator %s (at %p) leaked %zu bytes.\n",
                         FOONATHAN_MEMORY_LOG_PREFIX, info.name, info.allocator,
                         std::size_t(amount));
        else
            std::fprintf(stderr, "[%s] Allocator %s (at %p) has deallocated %zu bytes more than "
                                 "ever allocated "
                                 "(it's amazing you're able to see this message!).\n",
                         FOONATHAN_MEMORY_LOG_PREFIX, info.name, info.allocator,
                         std::size_t(-amount));
#else
        (void)info;
        (void)amount;
#endif
    }

    std::atomic<leak_handler> leak_h(default_leak_handler);
}

leak_handler foonathan::memory::set_leak_handler(leak_handler h)
{
    return leak_h.exchange(h ? h : default_leak_handler);
}

leak_handler foonathan::memory::get_leak_handler()
{
    return leak_h;
}

namespace
{
    void default_invalid_ptr_handler(const allocator_info& info, const void* ptr) FOONATHAN_NOEXCEPT
    {
#if FOONATHAN_HOSTED_IMPLEMENTATION
        std::fprintf(stderr, "[%s] Deallocation function of allocator %s (at %p) received invalid "
                             "pointer %p\n",
                     FOONATHAN_MEMORY_LOG_PREFIX, info.name, info.allocator, ptr);
#endif
        (void)info;
        (void)ptr;
        std::abort();
    }

    std::atomic<invalid_pointer_handler> invalid_ptr_h(default_invalid_ptr_handler);
}

invalid_pointer_handler foonathan::memory::set_invalid_pointer_handler(invalid_pointer_handler h)
{
    return invalid_ptr_h.exchange(h ? h : default_invalid_ptr_handler);
}

invalid_pointer_handler foonathan::memory::get_invalid_pointer_handler()
{
    return invalid_ptr_h;
}

namespace
{
    void default_buffer_overflow_handler(const void* memory, std::size_t node_size,
                                         const void* ptr) FOONATHAN_NOEXCEPT
    {
#if FOONATHAN_HOSTED_IMPLEMENTATION
        std::fprintf(stderr, "[%s] Buffer overflow at address %p detected, corresponding memory "
                             "block %p has only size %zu.",
                     FOONATHAN_MEMORY_LOG_PREFIX, ptr, memory, node_size);
#endif
        (void)memory;
        (void)node_size;
        (void)ptr;
        std::abort();
    }

    std::atomic<buffer_overflow_handler> buffer_overflow_h(default_buffer_overflow_handler);
}

buffer_overflow_handler foonathan::memory::set_buffer_overflow_handler(buffer_overflow_handler h)
{
    return buffer_overflow_h.exchange(h ? h : default_buffer_overflow_handler);
}

buffer_overflow_handler foonathan::memory::get_buffer_overflow_handler()
{
    return buffer_overflow_h;
}
