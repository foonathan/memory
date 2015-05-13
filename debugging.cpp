// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "debugging.hpp"

#include <atomic>
#include <cstdio>

using namespace foonathan::memory;

namespace
{
    void default_leak_handler(const char *name, void *allocator, std::size_t amount) FOONATHAN_NOEXCEPT
    {
        std::fprintf(stderr, "[foonathan::memory] Allocator %s (at %p) leaked %zu bytes.\n",
                            name, allocator, amount);
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
