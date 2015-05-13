// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "free_list.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <functional>
#include <utility>

#include "../debugging.hpp"

using namespace foonathan::memory;
using namespace detail;

namespace
{
    // pre: ptr
    char* get_next(void* ptr) FOONATHAN_NOEXCEPT
    {
        char* result = nullptr;
        std::memcpy(&result, ptr, sizeof(char*));
        return result;
    }

    // pre: ptr
    void set_next(void *ptr, char *next) FOONATHAN_NOEXCEPT
    {
        std::memcpy(ptr, &next, sizeof(char*));
    }

    // builds the list in memory block mem that is followed by no_blocks * node_size of memory
    void* build_list(std::size_t node_size, void *mem, std::size_t no_blocks) FOONATHAN_NOEXCEPT
    {
        assert(no_blocks != 0u);
        auto ptr = static_cast<char*>(mem);
        for (std::size_t i = 0u; i != no_blocks - 1; ++i, ptr += node_size)
            set_next(ptr, ptr + node_size);
        return ptr;
    }

    // pre: list, mem
    std::pair<void*, void*> find_position(void *list, void *mem) FOONATHAN_NOEXCEPT
    {
        auto greater = std::greater<char*>();
        auto prev = static_cast<char*>(list);
        if (greater(prev, static_cast<char*>(mem)))
            return std::make_pair(nullptr, list);

        auto ptr = get_next(prev);
        while (ptr)
        {
            if (greater(ptr, static_cast<char*>(mem)))
                break;
            prev = ptr;
            ptr = get_next(ptr);
        }

        return std::make_pair(prev, ptr);
    }

    // checks if a free memory block no_bytes bug starts at cur
    // pre: cur
    bool check_bytes(char* &cur, std::size_t no_bytes, std::size_t el_size) FOONATHAN_NOEXCEPT
    {
        auto available = el_size; // we already have one (cur)
        if (available >= no_bytes)
            return true;
        for (; cur; cur += el_size)
        {
            if (get_next(cur) == cur + el_size)
            {
                available += el_size;
                if (available >= no_bytes)
                    break;
            }
            else
                return false;
        }
        // get_next(cur) is the last element of the array, inclusive
        cur = get_next(cur);
        return true;
    }
}

FOONATHAN_CONSTEXPR std::size_t free_memory_list::min_element_size;
FOONATHAN_CONSTEXPR std::size_t free_memory_list::min_element_alignment;

free_memory_list::free_memory_list(std::size_t el_size) FOONATHAN_NOEXCEPT
: first_(nullptr),
  node_size_(std::max(min_element_size, el_size) + 2 * debug_fence_size),
  capacity_(0u)
{}

free_memory_list::free_memory_list(std::size_t el_size,
                                   void *mem, std::size_t size) FOONATHAN_NOEXCEPT
: free_memory_list(el_size)
{
    insert(mem, size);
}

free_memory_list::free_memory_list(free_memory_list &&other) FOONATHAN_NOEXCEPT
: first_(other.first_), node_size_(other.node_size_), capacity_(other.capacity_)
{
    other.first_ = nullptr;
    other.capacity_ = 0;
}

free_memory_list& free_memory_list::operator=(free_memory_list &&other) FOONATHAN_NOEXCEPT
{
    first_ = other.first_;
    node_size_ = other.node_size_;
    capacity_ = other.capacity_;
    other.first_ = nullptr;
    other.capacity_ = 0u;
    return *this;
}

void free_memory_list::insert(void *mem, std::size_t size) FOONATHAN_NOEXCEPT
{
    auto no_blocks = size / node_size_;
    capacity_ += no_blocks;
    auto last = build_list(node_size_, mem, no_blocks);
    set_next(last, first_);
    first_ = static_cast<char*>(mem);
}

void free_memory_list::insert_ordered(void *mem, std::size_t size) FOONATHAN_NOEXCEPT
{
    if (empty())
        return insert(mem, size);
    auto no_blocks = size / node_size_;
    capacity_ += no_blocks;
    auto pos = find_position(first_, mem);
    insert_between(pos.first, pos.second, mem, no_blocks);
}

void free_memory_list::insert_between(void *pre, void *after,
                                      void *mem, std::size_t no_blocks) FOONATHAN_NOEXCEPT
{
    auto last = build_list(node_size_, mem, no_blocks);

    if (pre)
        set_next(pre, static_cast<char*>(mem));
    else
        first_ = static_cast<char*>(mem);

    set_next(last, static_cast<char*>(after));
}

void* free_memory_list::allocate() FOONATHAN_NOEXCEPT
{
    --capacity_;
    auto block = first_;
    first_ = get_next(first_);

    debug_fill(block, debug_fence_size, debug_magic::fence_memory);
    block += debug_fence_size;
    debug_fill(block, node_size(), debug_magic::new_memory);
    debug_fill(block + node_size(), debug_fence_size, debug_magic::fence_memory);

    return block;
}

void* free_memory_list::allocate(std::size_t n, std::size_t other_node_size) FOONATHAN_NOEXCEPT
{
    auto n_bytes = n * other_node_size;
    auto needed = n_bytes + 2 * debug_fence_size;
    for(auto cur = first_; cur; cur = get_next(cur))
    {
        auto start = cur;
        if (check_bytes(cur, needed, node_size_))
        {
            // found continuos nodes big enough for need memory
            // cur is the last element inclusive, next(cur) is the next free node
            assert((cur - start) % node_size_ == 0u);
            auto no_nodes = (cur - start) / node_size_ + 1;
            capacity_ -= no_nodes;

            first_ = get_next(cur);

            debug_fill(start, debug_fence_size, debug_magic::fence_memory);
            start += debug_fence_size;
            debug_fill(start, n_bytes, debug_magic::new_memory);
            debug_fill(start + n_bytes, debug_fence_size, debug_magic::fence_memory);

            return start;
        }
    }
    return nullptr;
}

void free_memory_list::deallocate(void *ptr) FOONATHAN_NOEXCEPT
{
    debug_fill(ptr, node_size(), debug_magic::freed_memory);

    auto node_ptr = static_cast<char*>(ptr) - debug_fence_size;

    set_next(node_ptr, first_);
    first_ = node_ptr;
    ++capacity_;
}

void free_memory_list::deallocate(void *ptr, std::size_t n,
                                std::size_t other_node_size) FOONATHAN_NOEXCEPT
{
    auto n_bytes = n * other_node_size;
    debug_fill(ptr, n_bytes, debug_magic::freed_memory);

    auto node_ptr = static_cast<char*>(ptr) - debug_fence_size;
    auto bytes = n_bytes + 2 * debug_fence_size;
    auto no_blocks = bytes / node_size_ + (bytes % node_size_ != 0);

    auto last = build_list(node_size_, node_ptr, no_blocks);
    set_next(last, first_);
    first_ = node_ptr;
    capacity_ += no_blocks;
}

void free_memory_list::deallocate_ordered(void *ptr) FOONATHAN_NOEXCEPT
{
    debug_fill(ptr, node_size(), debug_magic::freed_memory);

    auto node_ptr = static_cast<char*>(ptr) - debug_fence_size;

    auto pos = find_position(first_, node_ptr);
    insert_between(pos.first, pos.second, node_ptr, 1);
    ++capacity_;
}

void free_memory_list::deallocate_ordered(void *ptr, std::size_t n,
                                        std::size_t other_node_size) FOONATHAN_NOEXCEPT
{
    auto n_bytes = n * other_node_size;
    debug_fill(ptr, n_bytes, debug_magic::freed_memory);

    auto node_ptr = static_cast<char*>(ptr) - debug_fence_size;
    auto bytes = n_bytes + 2 * debug_fence_size;
    auto no_blocks = bytes / node_size_ + (bytes % node_size_ != 0);

    auto pos = find_position(first_, node_ptr);
    insert_between(pos.first, pos.second, node_ptr, no_blocks);
    capacity_ += no_blocks;
}

std::size_t free_memory_list::node_size() const FOONATHAN_NOEXCEPT
{
    // note: node_size_ contains debug fence size, result not
    return node_size_ - 2 * debug_fence_size;
}
