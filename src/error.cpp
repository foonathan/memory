// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "error.hpp"

#include <atomic>
#include <cstdio>
#include <cstdlib>

using namespace foonathan::memory;

namespace
{
    void default_out_of_memory_handler(const allocator_info &info,
                                        std::size_t amount) FOONATHAN_NOEXCEPT
    {
        std::fprintf(stderr,
                "[%s] Allocator %s (at %p) ran out of memory trying to allocate %zu bytes.\n",
                FOONATHAN_MEMORY_IMPL_LOG_PREFIX, info.name, info.allocator, amount);
    }

    std::atomic<out_of_memory::handler> out_of_memory_h(default_out_of_memory_handler);
}

out_of_memory::handler out_of_memory::set_handler(out_of_memory::handler h)
{
    return out_of_memory_h.exchange(h);
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
        std::fprintf(stderr,
                "[%s] Allocator %s (at %p) received invalid size/alignment %zu, "
                "max supported is %zu",
                FOONATHAN_MEMORY_IMPL_LOG_PREFIX, info.name, info.allocator,
                passed, supported);
    }

    std::atomic<bad_allocation_size::handler>
            bad_alloc_size_h(default_bad_alloc_size_handler);
}

bad_allocation_size::handler bad_allocation_size::set_handler(
        bad_allocation_size::handler h)
{
    return bad_alloc_size_h.exchange(h);
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

void* foonathan::memory::detail::try_allocate(void* (* alloc_func)(size_t), std::size_t size,
                                              const allocator_info& info)
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
            FOONATHAN_THROW(out_of_memory(info, size));
    }
    FOONATHAN_MEMORY_UNREACHABLE("while (true) shouldn't exit");
    return nullptr;
}

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


void foonathan::memory::detail::handle_failed_assert(const char *msg,
                                                     const char *file, int line, const char *fnc) FOONATHAN_NOEXCEPT
{
    std::fprintf(stderr, "[%s] Assertion failure in function %s (%s:%d): %s.",
                          FOONATHAN_MEMORY_IMPL_LOG_PREFIX, fnc, file, line, msg);
    std::abort();
}
