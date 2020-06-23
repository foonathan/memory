// Copyright (C) 2015-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "virtual_memory.hpp"

#include "detail/debug_helpers.hpp"
#include "error.hpp"
#include "memory_arena.hpp"

using namespace foonathan::memory;

void detail::virtual_memory_allocator_leak_handler::operator()(std::ptrdiff_t amount)
{
    detail::debug_handle_memory_leak({FOONATHAN_MEMORY_LOG_PREFIX "::virtual_memory_allocator",
                                      nullptr},
                                     amount);
}

#if defined(_WIN32)
#include <windows.h>

namespace
{
    std::size_t get_page_size() noexcept
    {
        static_assert(sizeof(std::size_t) >= sizeof(DWORD), "possible loss of data");

        SYSTEM_INFO info;
        GetSystemInfo(&info);
        return std::size_t(info.dwPageSize);
    }
}

const std::size_t foonathan::memory::virtual_memory_page_size = get_page_size();

void* foonathan::memory::virtual_memory_reserve(std::size_t no_pages) noexcept
{
    auto pages =
    #if (_MSC_VER <= 1900)
        VirtualAlloc(nullptr, no_pages * virtual_memory_page_size, MEM_RESERVE, PAGE_READWRITE);
    #else
        VirtualAllocFromApp(nullptr, no_pages * virtual_memory_page_size, MEM_RESERVE, PAGE_READWRITE);
    #endif
    return pages;
}

void foonathan::memory::virtual_memory_release(void* pages, std::size_t) noexcept
{
    auto result = VirtualFree(pages, 0u, MEM_RELEASE);
    FOONATHAN_MEMORY_ASSERT_MSG(result, "cannot release pages");
}

void* foonathan::memory::virtual_memory_commit(void*       memory,
                                               std::size_t no_pages) noexcept
{
    auto region =
    #if (_MSC_VER <= 1900)
        VirtualAlloc(memory, no_pages * virtual_memory_page_size, MEM_COMMIT, PAGE_READWRITE);
    #else
        VirtualAllocFromApp(memory, no_pages * virtual_memory_page_size, MEM_COMMIT, PAGE_READWRITE);
    #endif
    if (!region)
        return nullptr;
    FOONATHAN_MEMORY_ASSERT(region == memory);
    return region;
}

void foonathan::memory::virtual_memory_decommit(void*       memory,
                                                std::size_t no_pages) noexcept
{
    auto result = VirtualFree(memory, no_pages * virtual_memory_page_size, MEM_DECOMMIT);
    FOONATHAN_MEMORY_ASSERT_MSG(result, "cannot decommit memory");
}
#elif defined(__unix__) || defined(__APPLE__) || defined(__VXWORKS__) // POSIX systems
#include <sys/mman.h>
#include <unistd.h>

#if defined(PAGESIZE)
const std::size_t foonathan::memory::virtual_memory_page_size = PAGESIZE;
#elif defined(PAGE_SIZE)
const std::size_t foonathan::memory::virtual_memory_page_size = PAGE_SIZE;
#else
const std::size_t foonathan::memory::virtual_memory_page_size = sysconf(_SC_PAGESIZE);
#endif

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

void* foonathan::memory::virtual_memory_reserve(std::size_t no_pages) noexcept
{
    auto pages = mmap(nullptr, no_pages * virtual_memory_page_size, PROT_NONE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return pages == MAP_FAILED ? nullptr : pages;
}

void foonathan::memory::virtual_memory_release(void* pages, std::size_t no_pages) noexcept
{
    auto result = munmap(pages, no_pages * virtual_memory_page_size);
    FOONATHAN_MEMORY_ASSERT_MSG(result == 0, "cannot release pages");
    (void)result;
}

void* foonathan::memory::virtual_memory_commit(void*       memory,
                                               std::size_t no_pages) noexcept
{
    auto size   = no_pages * virtual_memory_page_size;
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

void foonathan::memory::virtual_memory_decommit(void*       memory,
                                                std::size_t no_pages) noexcept
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
    (void)result;
}
#else
#warning "virtual memory functions not available on your platform, define your own"
#endif

namespace
{
    std::size_t calc_no_pages(std::size_t size) noexcept
    {
        auto div  = size / virtual_memory_page_size;
        auto rest = size % virtual_memory_page_size;

        return div + (rest != 0u) + (detail::debug_fence_size ? 2u : 1u);
    }
}

void* virtual_memory_allocator::allocate_node(std::size_t size, std::size_t)
{
    auto no_pages = calc_no_pages(size);
    auto pages    = virtual_memory_reserve(no_pages);
    if (!pages || !virtual_memory_commit(pages, no_pages))
        FOONATHAN_THROW(
            out_of_memory({FOONATHAN_MEMORY_LOG_PREFIX "::virtual_memory_allocator", nullptr},
                          no_pages * virtual_memory_page_size));
    on_allocate(size);

    return detail::debug_fill_new(pages, size, virtual_memory_page_size);
}

void virtual_memory_allocator::deallocate_node(void* node, std::size_t size,
                                               std::size_t) noexcept
{
    auto pages = detail::debug_fill_free(node, size, virtual_memory_page_size);

    on_deallocate(size);

    auto no_pages = calc_no_pages(size);
    virtual_memory_decommit(pages, no_pages);
    virtual_memory_release(pages, no_pages);
}

std::size_t virtual_memory_allocator::max_node_size() const noexcept
{
    return std::size_t(-1);
}

std::size_t virtual_memory_allocator::max_alignment() const noexcept
{
    return virtual_memory_page_size;
}

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
template class foonathan::memory::allocator_traits<virtual_memory_allocator>;
#endif

virtual_block_allocator::virtual_block_allocator(std::size_t block_size, std::size_t no_blocks)
: block_size_(block_size)
{
    FOONATHAN_MEMORY_ASSERT(block_size % virtual_memory_page_size == 0u);
    FOONATHAN_MEMORY_ASSERT(no_blocks > 0);
    auto total_size = block_size_ * no_blocks;
    auto no_pages   = total_size / virtual_memory_page_size;

    cur_ = static_cast<char*>(virtual_memory_reserve(no_pages));
    if (!cur_)
        FOONATHAN_THROW(out_of_memory(info(), total_size));
    end_ = cur_ + total_size;
}

virtual_block_allocator::~virtual_block_allocator() noexcept
{
    virtual_memory_release(cur_, (end_ - cur_) / virtual_memory_page_size);
}

memory_block virtual_block_allocator::allocate_block()
{
    if (std::size_t(end_ - cur_) < block_size_)
        FOONATHAN_THROW(out_of_fixed_memory(info(), block_size_));
    auto mem = virtual_memory_commit(cur_, block_size_ / virtual_memory_page_size);
    if (!mem)
        FOONATHAN_THROW(out_of_fixed_memory(info(), block_size_));
    cur_ += block_size_;
    return {mem, block_size_};
}

void virtual_block_allocator::deallocate_block(memory_block block) noexcept
{
    detail::
        debug_check_pointer([&] { return static_cast<char*>(block.memory) == cur_ - block_size_; },
                            info(), block.memory);
    cur_ -= block_size_;
    virtual_memory_decommit(cur_, block_size_);
}

allocator_info virtual_block_allocator::info() noexcept
{
    return {FOONATHAN_MEMORY_LOG_PREFIX "::virtual_block_allocator", this};
}
