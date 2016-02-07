// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "error.hpp"

#include <atomic>

#if FOONATHAN_HOSTED_IMPLEMENTATION
    #include <cstdio>
#endif

#include FOONATHAN_GET_NEW_HANDLER_HEADER

using namespace foonathan::memory;

namespace
{
    void default_out_of_memory_handler(const allocator_info &info,
                                        std::size_t amount) FOONATHAN_NOEXCEPT
    {
    #if FOONATHAN_HOSTED_IMPLEMENTATION
        std::fprintf(stderr,
                "[%s] Allocator %s (at %p) ran out of memory trying to allocate %zu bytes.\n",
                     FOONATHAN_MEMORY_LOG_PREFIX, info.name, info.allocator, amount);
    #endif
    }

    std::atomic<out_of_memory::handler> out_of_memory_h(default_out_of_memory_handler);
}

out_of_memory::handler out_of_memory::set_handler(out_of_memory::handler h)
{
    return out_of_memory_h.exchange(h ? h : default_out_of_memory_handler);
}

out_of_memory::handler out_of_memory::get_handler()
{
    return out_of_memory_h;
}

out_of_memory::out_of_memory(const allocator_info& info, std::size_t amount)
: info_(info), amount_(amount)
{
    out_of_memory_h.load()(info, amount);
}

const char* out_of_memory::what() const FOONATHAN_NOEXCEPT
{
    return "out of memory";
}

namespace
{
    void default_bad_alloc_size_handler(const allocator_info &info,
                                        std::size_t passed,
                                        std::size_t supported) FOONATHAN_NOEXCEPT
    {
    #if FOONATHAN_HOSTED_IMPLEMENTATION
        std::fprintf(stderr,
                "[%s] Allocator %s (at %p) received invalid size/alignment %zu, "
                "max supported is %zu",
                     FOONATHAN_MEMORY_LOG_PREFIX, info.name, info.allocator,
                passed, supported);
    #endif
    }

    std::atomic<bad_allocation_size::handler>
            bad_alloc_size_h(default_bad_alloc_size_handler);
}

bad_allocation_size::handler bad_allocation_size::set_handler(
        bad_allocation_size::handler h)
{
    return bad_alloc_size_h.exchange(h ? h : default_bad_alloc_size_handler);
}

bad_allocation_size::handler bad_allocation_size::get_handler()
{
    return bad_alloc_size_h;
}

bad_allocation_size::bad_allocation_size(const allocator_info& info,
                                         std::size_t passed,
                                         std::size_t supported)
: info_(info), passed_(passed), supported_(supported)
{
    bad_alloc_size_h.load()(info_, passed_, supported_);
}

const char* bad_allocation_size::what() const FOONATHAN_NOEXCEPT
{
    return "allocation size/alignment exceeds supported maximum for allocator";
}
