// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "virtual_memory.hpp"

#include "error.hpp"

using namespace foonathan::memory;

#if defined(_WIN32)
    #include <windows.h>

    namespace
    {
        std::size_t get_page_size() FOONATHAN_NOEXCEPT
        {
            static_assert(sizeof(std::size_t) >= sizeof(DWORD), "possible loss of data");

            SYSTEM_INFO info;
            GetSystemInfo(&info);
            return std::size_t(info.dwPageSize);
        }
    }

    const std::size_t foonathan::memory::virtual_memory_page_size = get_page_size();

    void* foonathan::memory::virtual_memory_reserve(std::size_t no_pages) FOONATHAN_NOEXCEPT
    {
        auto pages = VirtualAlloc(nullptr, no_pages * virtual_memory_page_size, MEM_RESERVE, PAGE_READWRITE);
        return pages;
    }

    void foonathan::memory::virtual_memory_release(void *pages, std::size_t) FOONATHAN_NOEXCEPT
    {
        auto result = VirtualFree(pages, 0u, MEM_RELEASE);
        FOONATHAN_MEMORY_ASSERT_MSG(result, "cannot release pages");
    }

    void* foonathan::memory::virtual_memory_commit(void *memory, std::size_t no_pages) FOONATHAN_NOEXCEPT
    {
        auto region = VirtualAlloc(memory, no_pages * virtual_memory_page_size, MEM_COMMIT, PAGE_READWRITE);
        if (!region)
            return nullptr;
        FOONATHAN_MEMORY_ASSERT(region == memory);
        return region;
    }

    void foonathan::memory::virtual_memory_decommit(void *memory, std::size_t no_pages) FOONATHAN_NOEXCEPT
    {
        auto result = VirtualFree(memory, no_pages * virtual_memory_page_size, MEM_DECOMMIT);
        FOONATHAN_MEMORY_ASSERT_MSG(result, "cannot decommit memory");
    }
#elif defined(__unix__) || defined(__APPLE__) // POSIX systems
    #include <sys/mman.h>
    #include <unistd.h>

    #if defined(PAGESIZE)
        const std::size_t foonathan::memory::virtual_memory_page_size = PAGESIZE;
    #elif defined(PAGE_SIZE)
        const std::size_t foonathan::memory::virtual_memory_page_size = PAGE_SIZE;
    #else
        const std::size_t foonathan::memory::virtual_memory_page_size = sysconf(_SC_PAGESIZE);
    #endif

    void* foonathan::memory::virtual_memory_reserve(std::size_t no_pages) FOONATHAN_NOEXCEPT
    {
        auto pages = mmap(nullptr, no_pages * virtual_memory_page_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        return pages == MAP_FAILED ? nullptr : pages;
    }

    void foonathan::memory::virtual_memory_release(void *pages, std::size_t no_pages) FOONATHAN_NOEXCEPT
    {
        auto result = munmap(pages, no_pages * virtual_memory_page_size);
        FOONATHAN_MEMORY_ASSERT_MSG(result == 0, "cannot release pages");
    }

    void* foonathan::memory::virtual_memory_commit(void *memory, std::size_t no_pages) FOONATHAN_NOEXCEPT
    {
        auto size = no_pages * virtual_memory_page_size;
        auto result = mprotect(memory, size, PROT_WRITE | PROT_READ);
        if (result != 0u)
            return nullptr;

        // advise that the memory will be needed
        #if defined(MADV_WILLNEED)
            madvise(memory, size, MADV_WILLNEED);
        #elif defined(POSIX_MADV_WILLNEED)
            posix_madvise(memory, size, POSIX_MADV_WILLNEED);
        #endif

        return memory;
    }

    void foonathan::memory::virtual_memory_decommit(void *memory, std::size_t no_pages) FOONATHAN_NOEXCEPT
    {
         auto size = no_pages * virtual_memory_page_size;
        // advise that the memory won't be needed anymore
        #if defined(MADV_FREE)
            madvise(memory, size, MADV_FREE);
        #elif defined(MADV_DONTNEED)
            madvise(memory, size, MADV_DONTNEED);
        #elif defined(POSIX_MADV_DONTNEED)
            posix_madvise(memory, size, POSIX_MADV_DONTNEED);
        #endif

        auto result = mprotect(memory, size, PROT_NONE);
        FOONATHAN_MEMORY_ASSERT_MSG(result == 0, "cannot decommit memory");
    }
#else
    #warning "virtual memory functions not available on your platform, define your own"
#endif
