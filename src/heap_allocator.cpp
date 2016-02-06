// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "heap_allocator.hpp"

#include <new>

#include "detail/align.hpp"
#include "debugging.hpp"
#include "error.hpp"

using namespace foonathan::memory;

#ifdef _WIN32
    #include <malloc.h>
    #include <windows.h>

    namespace
    {
        HANDLE get_process_heap() FOONATHAN_NOEXCEPT
        {
            static auto heap = GetProcessHeap();
            return heap;
        }

        std::size_t max_size() FOONATHAN_NOEXCEPT
        {
            return _HEAP_MAXREQ;
        }
    }

    void* foonathan::memory::heap_alloc(std::size_t size) FOONATHAN_NOEXCEPT
    {
        return HeapAlloc(get_process_heap(), 0, size);
    }

    void foonathan::memory::heap_dealloc(void *ptr, std::size_t) FOONATHAN_NOEXCEPT
    {
        HeapFree(get_process_heap(), 0, ptr);
    }

#elif FOONATHAN_HOSTED_IMPLEMENTATION
    #include <cstdlib>
    #include <memory>

    void* foonathan::memory::heap_alloc(std::size_t size) FOONATHAN_NOEXCEPT
    {
        return std::malloc(size);
    }

    void foonathan::memory::heap_dealloc(void *ptr, std::size_t) FOONATHAN_NOEXCEPT
    {
        std::free(ptr);
    }

    namespace
    {
        std::size_t max_size() FOONATHAN_NOEXCEPT
        {
            return std::allocator<char>().max_size();
        }
    }
#else
    // no implementation for heap_alloc/heap_dealloc

    namespace
    {
        std::size_t max_size() FOONATHAN_NOEXCEPT
        {
            return std::size_t(-1);
        }
    }
#endif

std::size_t detail::heap_allocator_impl::max_node_size() FOONATHAN_NOEXCEPT
{
    return max_size();
}

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
    template class foonathan::memory::allocator_traits<heap_allocator>;
#endif
