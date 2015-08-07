// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "debugging.hpp"

#include <atomic>
#include <cstdlib>

#if FOONATHAN_HOSTED_IMPLEMENTATION
    #include <cstdio>
#endif


using namespace foonathan::memory;

namespace
{
    void default_leak_handler(const allocator_info &info, std::size_t amount) FOONATHAN_NOEXCEPT
    {
    #if FOONATHAN_HOSTED_IMPLEMENTATION
        std::fprintf(stderr, "[%s] Allocator %s (at %p) leaked %zu bytes.\n",
                     FOONATHAN_MEMORY_LOG_PREFIX, info.name, info.allocator, amount);
    #endif
    }

    std::atomic<leak_handler> leak_h(default_leak_handler);
}

leak_handler foonathan::memory::set_leak_handler(leak_handler h)
{
    return leak_h.exchange(h);
}

leak_handler foonathan::memory::get_leak_handler()
{
    return leak_h;
}

namespace
{
    void default_invalid_ptr_handler(const allocator_info &info, const void *ptr) FOONATHAN_NOEXCEPT
    {
    #if FOONATHAN_HOSTED_IMPLEMENTATION
        std::fprintf(stderr, "[%s] Deallocation function of allocator %s (at %p) received invalid pointer %p\n",
                     FOONATHAN_MEMORY_LOG_PREFIX, info.name, info.allocator, ptr);
    #endif
        std::abort();
    }

    std::atomic<invalid_pointer_handler> invalid_ptr_h(default_invalid_ptr_handler);
}

invalid_pointer_handler foonathan::memory::set_invalid_pointer_handler(invalid_pointer_handler h)
{
    return invalid_ptr_h.exchange(h);
}

invalid_pointer_handler foonathan::memory::get_invalid_pointer_handler()
{
    return invalid_ptr_h;
}
