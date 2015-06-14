// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "error.hpp"

#include <atomic>
#include <cassert>
#include <cstdio>

using namespace foonathan::memory;

#if FOONATHAN_HAS_GET_NEW_HANDLER
    std::new_handler foonathan::memory::detail::get_new_handler()
    {
        return std::get_new_handler();
    }
#else
    std::new_handler foonathan::memory::detail::get_new_handler()
    {
        auto handler = std::set_new_handler(nullptr);
        std::set_new_handler(handler);
        return handler;
    }
#endif

void* foonathan::memory::detail::try_allocate(void* (*alloc_func)(std::size_t), std::size_t size,
                                            const char *name, void *allocator)
{
    while (true)
    {
        auto memory = alloc_func(size);
        if (memory)
            return memory;

        auto handler = detail::get_new_handler();
        if (handler)
            handler();
        else
        {
            get_out_of_memory_handler()(name, allocator, size);
            throw std::bad_alloc();
        }
    }
    assert(false);
    return nullptr;
}

namespace
{
    void default_out_of_memory_handler(const char *name, void *allocator,
                                        std::size_t amount) FOONATHAN_NOEXCEPT
    {
        std::fprintf(stderr,
                "[foonathan::memory] Allocator %s (at %p) ran out of memory trying to allocate %zu bytes.\n",
                name, allocator, amount);
    }

    std::atomic<out_of_memory_handler> out_of_memory_h(default_out_of_memory_handler);
}

out_of_memory_handler foonathan::memory::set_out_of_memory_handler(out_of_memory_handler h)
{
    return out_of_memory_h.exchange(h);
}

out_of_memory_handler foonathan::memory::get_out_of_memory_handler()
{
    return out_of_memory_h;
}
