// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "detail/debug_helpers.hpp"

#if FOONATHAN_HOSTED_IMPLEMENTATION
    #include <cstring>
#endif

#include "debugging.hpp"

using namespace foonathan::memory;
using namespace detail;

#if FOONATHAN_MEMORY_DEBUG_FILL
    void detail::debug_fill(void *memory, std::size_t size, debug_magic m) FOONATHAN_NOEXCEPT
    {
        #if FOONATHAN_HOSTED_IMPLEMENTATION
            std::memset(memory, static_cast<int>(m), size);
        #else
            // do the naive loop :(
            auto ptr = static_cast<unsigned char*>(memory);
            for (std::size_t i = 0u; i != size; ++i)
                *ptr++ = static_cast<unsigned char>(m);
        #endif
    }

    void* detail::debug_is_filled(void *memory, std::size_t size, debug_magic m) FOONATHAN_NOEXCEPT
    {
        auto byte = static_cast<unsigned char*>(memory);
        for (auto end = byte + size; byte != end; ++byte)
            if (*byte != static_cast<unsigned char>(m))
                return byte;
        return nullptr;
    }

    void* detail::debug_fill_new(void *memory,
                         std::size_t node_size, std::size_t fence_size) FOONATHAN_NOEXCEPT
    {
        if (!debug_fence_size)
            fence_size = 0u; // force override of fence_size

        auto mem = static_cast<char*>(memory);
        debug_fill(mem, fence_size, debug_magic::fence_memory);

        mem += fence_size;
        debug_fill(mem, node_size, debug_magic::new_memory);

        debug_fill(mem + node_size, fence_size, debug_magic::fence_memory);

        return mem;
    }

    void* detail::debug_fill_free(void *memory,
                            std::size_t node_size, std::size_t fence_size) FOONATHAN_NOEXCEPT
    {
        if (!debug_fence_size)
            fence_size = 0u; // force override of fence_size

        debug_fill(memory, node_size, debug_magic::freed_memory);

        auto pre_fence = static_cast<unsigned char*>(memory) - fence_size;
        if (auto pre_dirty = debug_is_filled(pre_fence, fence_size, debug_magic::fence_memory))
            get_buffer_overflow_handler()(memory, node_size, pre_dirty);

        auto post_mem = static_cast<unsigned char*>(memory) + node_size;
        if (auto post_dirty = debug_is_filled(post_mem, fence_size, debug_magic::fence_memory))
            get_buffer_overflow_handler()(memory, node_size, post_dirty);

        return pre_fence;
    }

    void detail::debug_fill_internal(void *memory, std::size_t size, bool free) FOONATHAN_NOEXCEPT
    {
        debug_fill(memory, size, free ? debug_magic::internal_freed_memory : debug_magic::internal_memory);
    }
#else
    void detail::debug_fill(void *, std::size_t, debug_magic) FOONATHAN_NOEXCEPT {}

    void* detail::debug_is_filled(void *, std::size_t, debug_magic) FOONATHAN_NOEXCEPT
    {
        return nullptr;
    }

    void* detail::debug_fill_new(void *memory, std::size_t, std::size_t) FOONATHAN_NOEXCEPT
    {
        return memory;
    }

    void* detail::debug_fill_free(void *memory, std::size_t, std::size_t) FOONATHAN_NOEXCEPT
    {
        return static_cast<char*>(memory);
    }

    void detail::debug_fill_internal(void *, std::size_t, bool) FOONATHAN_NOEXCEPT {}
#endif

void detail::debug_handle_invalid_ptr(const allocator_info &info, void *ptr)
{
    get_invalid_pointer_handler()(info, ptr);
}

void detail::debug_handle_memory_leak(const allocator_info &info, std::ptrdiff_t amount)
{
    get_leak_handler()(info, amount);
}
