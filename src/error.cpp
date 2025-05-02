// Copyright (C) 2015-2025 Jonathan Müller and foonathan/memory contributors
// SPDX-License-Identifier: Zlib

#include "error.hpp"

#include <atomic>

#if FOONATHAN_HOSTED_IMPLEMENTATION
#include <cstdio>
#endif

using namespace foonathan::memory;

namespace
{
    void default_out_of_memory_handler(const allocator_info& info, std::size_t amount) noexcept
    {
#if FOONATHAN_HOSTED_IMPLEMENTATION
        std::fprintf(stderr,
                     "[%s] Allocator %s (at %p) ran out of memory trying to allocate %zu bytes.\n",
                     FOONATHAN_MEMORY_LOG_PREFIX, info.name, info.allocator, amount);
#endif
    }

    std::atomic<out_of_memory::handler> out_of_memory_h(default_out_of_memory_handler);
} // namespace

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

const char* out_of_memory::what() const noexcept
{
    return "low-level allocator is out of memory";
}

const char* out_of_fixed_memory::what() const noexcept
{
    return "fixed size allocator is out of memory";
}

namespace
{
    void default_bad_alloc_size_handler(const allocator_info& info, std::size_t passed,
                                        std::size_t supported) noexcept
    {
#if FOONATHAN_HOSTED_IMPLEMENTATION
        std::fprintf(stderr,
                     "[%s] Allocator %s (at %p) received invalid size/alignment %zu, "
                     "max supported is %zu\n",
                     FOONATHAN_MEMORY_LOG_PREFIX, info.name, info.allocator, passed, supported);
#endif
    }

    std::atomic<bad_allocation_size::handler> bad_alloc_size_h(default_bad_alloc_size_handler);
} // namespace

bad_allocation_size::handler bad_allocation_size::set_handler(bad_allocation_size::handler h)
{
    return bad_alloc_size_h.exchange(h ? h : default_bad_alloc_size_handler);
}

bad_allocation_size::handler bad_allocation_size::get_handler()
{
    return bad_alloc_size_h;
}

bad_allocation_size::bad_allocation_size(const allocator_info& info, std::size_t passed,
                                         std::size_t supported)
: info_(info), passed_(passed), supported_(supported)
{
    bad_alloc_size_h.load()(info_, passed_, supported_);
}

const char* bad_allocation_size::what() const noexcept
{
    return "allocation node size exceeds supported maximum of allocator";
}

const char* bad_node_size::what() const noexcept
{
    return "allocation node size exceeds supported maximum of allocator";
}

const char* bad_array_size::what() const noexcept
{
    return "allocation array size exceeds supported maximum of allocator";
}

const char* bad_alignment::what() const noexcept
{
    return "allocation alignment exceeds supported maximum of allocator";
}
