// Copyright (C) 2015-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "heap_allocator.hpp"

#include "error.hpp"

using namespace foonathan::memory;

#ifdef _WIN32
#include <malloc.h>
#include <windows.h>

namespace
{
    HANDLE get_process_heap() noexcept
    {
        static auto heap = GetProcessHeap();
        return heap;
    }

    std::size_t max_size() noexcept
    {
        return _HEAP_MAXREQ;
    }
}

void* foonathan::memory::heap_alloc(std::size_t size) noexcept
{
    return HeapAlloc(get_process_heap(), 0, size);
}

void foonathan::memory::heap_dealloc(void* ptr, std::size_t) noexcept
{
    HeapFree(get_process_heap(), 0, ptr);
}

#elif FOONATHAN_HOSTED_IMPLEMENTATION
#include <cstdlib>
#include <memory>

void* foonathan::memory::heap_alloc(std::size_t size) noexcept
{
    return std::malloc(size);
}

void foonathan::memory::heap_dealloc(void* ptr, std::size_t) noexcept
{
    std::free(ptr);
}

namespace
{
    std::size_t max_size() noexcept
    {
        return std::allocator_traits<std::allocator<char>>::max_size({});
    }
}
#else
// no implementation for heap_alloc/heap_dealloc

namespace
{
    std::size_t max_size() noexcept
    {
        return std::size_t(-1);
    }
}
#endif

allocator_info detail::heap_allocator_impl::info() noexcept
{
    return {FOONATHAN_MEMORY_LOG_PREFIX "::heap_allocator", nullptr};
}

std::size_t detail::heap_allocator_impl::max_node_size() noexcept
{
    return max_size();
}

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
template class detail::lowlevel_allocator<detail::heap_allocator_impl>;
template class foonathan::memory::allocator_traits<heap_allocator>;
#endif
